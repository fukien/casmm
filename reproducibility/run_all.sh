#!/usr/bin/env bash
#
# One-click reproduction of all experiments.
#
#   bash run_all.sh                    # run every stage, in dependency order
#   bash run_all.sh figure14 table3    # run only the named stages
#   bash run_all.sh --list             # show the stage -> script-folder mapping
#
# A stage that fails is recorded and the run continues; the failed stages are
# listed again at the end.  Per-stage stdout is captured under
# ../logs/run_all-logs/<stage>.log; the individual scripts keep writing their
# own logs into ../logs/<yyyymmdd>-logs/ as before.
#
# Datasets are NOT generated here -- see scripts/20260121-scripts (R-MAT /
# SuiteSparse).  The two stages that need their own external data (figure14,
# table3) fetch it themselves; both need network access.

set -u

date
start_time=$(date +%s)

CUR_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd -- "${CUR_DIR}/.." && pwd)
SCRIPTS_DIR=${REPO_ROOT}/scripts
LOG_DIR=${REPO_ROOT}/logs/run_all-logs
mkdir -p ${LOG_DIR}

PY=$(command -v python3 || command -v python)
if [ -z "$PY" ]; then
	echo "ERROR: no python on PATH"
	exit 1
fi

# stage name -> script folder, in the order they must run.  "base" produces the
# raw SpGEMM logs (20260122..20260125) that figures 8/9/11/12/13 soft-link to,
# so it has to come first.
STAGES=(
	"base:20260122,20260123,20260124,20260125-scripts"
	"figure9:20260129-scripts"
	"figure8:20260202-scripts"
	"figure10:20260126-scripts"
	"figure11:20260130-scripts"
	"figure12:20260127-scripts"
	"figure13:20260131-scripts"
	"figure1:20260205-scripts"
	"figure3:20260204-scripts"
	"figure4:20260203-scripts"
	"figure6:20260201-scripts"
	"figure14:20260207-scripts"
	"table3:20260208-scripts"
)

failed=()


banner()
{
	echo
	echo "############################################################"
	echo "### $*"
	echo "############################################################"
	date
}


# run_in <script-folder> <log-name> <command...>
run_in()
{
	local dir=$1; shift
	local name=$1; shift
	local log=${LOG_DIR}/${name}.log

	if [ ! -d "${SCRIPTS_DIR}/${dir}" ]; then
		echo "   [FAIL] ${SCRIPTS_DIR}/${dir} does not exist"
		return 1
	fi

	echo "   ${dir}: $* -- log: ${log}"
	(cd ${SCRIPTS_DIR}/${dir} && "$@") > ${log} 2>&1
	local rc=$?
	if [ $rc -eq 0 ]; then
		echo "   ok."
	else
		echo "   [FAIL] rc=${rc} -- see ${log}"
	fi
	return $rc
}


# --------------------------------------------------------------------------
# base: raw SpGEMM runs that the soft-linking figures depend on.
# (20260129-scripts/driver.sh is not used here: it chains these four folders
# but leaves the shell in 20260125-scripts before calling its own softlink.sh.)
# --------------------------------------------------------------------------
stage_base()
{
	local rc=0
	for d in 20260122 20260123 20260124 20260125; do
		run_in ${d}-scripts base_${d} bash driver.sh || rc=1
	done
	return $rc
}


stage_figure9()  { run_in 20260129-scripts figure9  bash softlink.sh; }
stage_figure8()  { run_in 20260202-scripts figure8  bash softlink.sh; }
stage_figure10() { run_in 20260126-scripts figure10 bash driver.sh; }
stage_figure11() { run_in 20260130-scripts figure11 bash driver.sh; }
stage_figure12() { run_in 20260127-scripts figure12 bash driver.sh; }
stage_figure13() { run_in 20260131-scripts figure13 bash driver.sh; }
stage_figure1()  { run_in 20260205-scripts figure1  bash test_g500_17_16.sh; }
stage_figure3()  { run_in 20260204-scripts figure3  bash in_debug_test.sh; }
stage_figure4()  { run_in 20260203-scripts figure4  bash test_ab_hash_hyper.sh; }
stage_figure6()  { run_in 20260201-scripts figure6  bash test_tamu.sh; }


# --------------------------------------------------------------------------
# figure14: join-aggregate / join-project.  Its driver prepares its own data
# (IMDB, GroupLens, ClickBench) and skips whatever it cannot build.  Plotting
# is left to plot_all.sh.
# --------------------------------------------------------------------------
stage_figure14()
{
	run_in 20260207-scripts figure14 env SKIP_PLOT=1 bash driver.sh
}


# --------------------------------------------------------------------------
# table3: PathSim on DBLP V10.  run_tab3.sh does no data preparation and aborts
# if the matrices are missing, so fetch + convert them first (~1.8 GB download,
# resumable).  The table is printed to stdout by run_tab3.sh -- it lands in
# ../logs/run_all-logs/table3.log here, and in ../logs/tab3-logs/ as markdown.
# --------------------------------------------------------------------------
stage_table3()
{
	local ds_dir=${REPO_ROOT}/dataset/pathsim/dblp_v10

	if [ ! -d "${ds_dir}" ]; then
		echo "   dblp_v10 not found -- preparing it first"
		run_in 20260208-scripts table3_fetch bash fetch_dblp_v10.sh || return 1
		run_in 20260208-scripts table3_prep  $PY prep_dblp_v10.py    || return 1
		run_in 20260208-scripts table3_mtx   $PY dblp2mtx.py --name dblp_v10 || return 1
	else
		echo "   dblp_v10 already built, skipping data prep"
	fi

	run_in 20260208-scripts table3 bash run_tab3.sh || return 1

	echo
	echo "   --- Table 3 ---"
	sed -n '/^Table 3:/,$p' ${LOG_DIR}/table3.log
}


list_stages()
{
	printf "%-10s %s\n" "STAGE" "SCRIPT FOLDER"
	for entry in "${STAGES[@]}"; do
		printf "%-10s %s\n" "${entry%%:*}" "${entry#*:}"
	done
}


# --------------------------------------------------------------------------

if [ $# -gt 0 ] && { [ "$1" = "--list" ] || [ "$1" = "-l" ]; }; then
	list_stages
	exit 0
fi

if [ $# -gt 0 ]; then
	selected=("$@")
else
	selected=()
	for entry in "${STAGES[@]}"; do
		selected+=("${entry%%:*}")
	done
fi

for name in "${selected[@]}"; do
	if ! declare -F stage_${name} > /dev/null; then
		echo "ERROR: unknown stage '${name}'"
		list_stages
		exit 1
	fi
done

echo "stages: ${selected[*]}"

for name in "${selected[@]}"; do
	banner "${name}"
	stage_${name} || failed+=("${name}")
done


echo
if [ ${#failed[@]} -gt 0 ]; then
	echo "########## stages that failed ##########"
	for x in "${failed[@]}"; do echo "   ${x}  -- ${LOG_DIR}/${x}.log"; done
else
	echo "all stages completed."
fi
echo "logs: ${LOG_DIR}"

end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date

[ ${#failed[@]} -eq 0 ]
