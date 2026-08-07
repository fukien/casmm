#!/usr/bin/env bash

set -u

date
start_time=$(date +%s)

CUR_DIR=$(pwd)
DATASET_DIR=${CUR_DIR}/../../dataset/groupby-sorted
RAW_DIR=${CUR_DIR}/../../dataset/groupby-raw
CHBC_RAW=${RAW_DIR}/clickbench_user_url_counts_converted.tsv

SKIP_DATA=${SKIP_DATA:-}
FORCE_DATA=${FORCE_DATA:-}
SKIP_PLOT=${SKIP_PLOT:-}

PY=$(command -v python3 || command -v python)
if [ -z "$PY" ]; then
	echo "ERROR: no python on PATH"
	exit 1
fi


have()
{
	[ -f "${DATASET_DIR}/$1-sorted.org.bin" ]
}


need()
{
	[ -n "$FORCE_DATA" ] && return 0
	have "$1" && return 1
	return 0
}


stage()
{
	echo
	echo "############################################################"
	echo "### $*"
	echo "############################################################"
}


run_stage()
{
	local name=$1; shift
	stage "$name"
	if "$@" > ${name}.log 2>&1; then
		echo "   ok. log: ${CUR_DIR}/${name}.log"
	else
		echo "   [FAIL] rc=$? -- see ${CUR_DIR}/${name}.log"
		return 1
	fi
}


if [ -n "$SKIP_DATA" ]; then
	stage "data prep -- skipped (SKIP_DATA)"
else
	if need imdb_movie; then
		run_stage get_imdb bash -c \
			'bash download_imdb.sh && '"$PY"' imdb2mtx.py --name imdb_movie --title-types movie --format bin' \
			|| exit 1
	else
		echo "data: imdb_movie already built, skipping"
	fi

	if need bkgn || need ml20m; then
		run_stage get_groupby bash get_groupby.sh || exit 1
	else
		echo "data: bkgn + ml20m already built, skipping"
	fi

	if need chbc_s_uni || need chbc_s_skew; then
		if [ -f "$CHBC_RAW" ]; then
			if [ ! -f "${DATASET_DIR}/clickbench-sorted.csv" ] || [ -n "$FORCE_DATA" ]; then
				run_stage get_clickbench bash get_clickbench.sh || exit 1
			fi
			run_stage get_clickbench_sampled bash get_clickbench_sampled.sh || exit 1
		else
			echo "data: !! ${CHBC_RAW} not found --"
			echo "      chbc_s_uni / chbc_s_skew will be skipped by every run script."
		fi
	else
		echo "data: chbc_s_uni + chbc_s_skew already built, skipping"
	fi
fi

echo
echo "datasets present in ${DATASET_DIR}:"
for d in imdb_movie chbc_s_uni bkgn chbc_s_skew ml20m; do
	have $d && echo "   $d" || echo "   $d  -- MISSING, will be skipped"
done


failed=()
for s in groupby_ab groupby_others groupby_mkl groupby_dim3; do
	run_stage $s bash ${s}.sh || failed+=("$s")
done


if [ -n "$SKIP_PLOT" ]; then
	stage "plot -- skipped (SKIP_PLOT)"
else
	run_stage plot_groupby_jp $PY plot_groupby_jp.py || failed+=("plot_groupby_jp")
	echo
	ls -la ${CUR_DIR}/../../figs/figure14-figs/ 2> /dev/null
fi


if [ ${#failed[@]} -gt 0 ]; then
	echo
	echo "########## stages that failed ##########"
	for x in "${failed[@]}"; do echo "   $x  -- ${CUR_DIR}/${x}.log"; done
fi

end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date

[ ${#failed[@]} -eq 0 ]
