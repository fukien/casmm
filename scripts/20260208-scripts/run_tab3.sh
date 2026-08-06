#!/usr/bin/env bash
set -u


date
start_time=$(date +%s)

RUN_NUM=3
NUMACTL="/home/huang/workspace/numactl/numactl"

CUR_DIR=$(pwd)

DIR_PATH=${CUR_DIR}/../../logs/tab3-logs
FIG_PATH=${CUR_DIR}/../../figs/tab3-figs
mkdir -p $DIR_PATH $FIG_PATH

DATA_DIR=${CUR_DIR}/../../dataset/pathsim
DS_NAME=dblp_v10

PY=$(command -v python3 || command -v python)
if [ -z "$PY" ]; then
	echo "ERROR: no python on PATH"
	exit 1
fi

if [ ! -d "${DATA_DIR}/${DS_NAME}" ]; then
	echo "ERROR: dataset not found at ${DATA_DIR}/${DS_NAME}"
	echo "Run: python scripts/20260208-scripts/dblp2mtx.py --name ${DS_NAME}"
	exit 1
fi

QUERY_RUNS=(
	"APCPA|30247|10"
	"APTPA|30247|10"
	"TPAPT|105634|10"
	"CPAPC|0|10"
)
METAPATHS=APCPA,APTPA,TPAPT,CPAPC

BACKENDS=(
	ab_hashspgemm_pathsim
	mkl_dcsrmultcsr_pathsim
)

CORE_RANGE="0-31,64-95"
THREAD_NUM=$(numactl --hardware | awk '/node 0 cpus:/ {print (NF-3)/2}')
THREAD_NUM=$((2 * THREAD_NUM))
AB_HIGH_THREAD_NUM=22
AB_PRT=0.85
DAHA_SYMB_THRES=7000000
DAHA_NUMC_THRES=2100000
AB_ACC_THRES=1.116000

echo "CPU binding: ${CORE_RANGE}"
echo "threads: ${THREAD_NUM}"

cd ../..
bash clean.sh
mkdir build
cd build
cmake .. -DTHREAD_NUM=$THREAD_NUM -DUSE_HYPERTHREADING=true \
	-DNUMA_MASK_VAL=1 -DUSE_WEIGHTED_INTERLEAVING=false -DUSE_INTERLEAVING=false \
	-DCSRC_WRITE_OPTIMIZED=false -DCSRC_PREPOPULATED=false -DUSE_HUGE=false \
	-DMEMSV=false -DMEMSVB=false -DMEM_MON=false \
	-DAB_HIGH_THREAD_NUM=$AB_HIGH_THREAD_NUM -DAB_SIZE=524288 -DAB_MIN_NUM=32768 \
	-DAB_SIMP=false -DAB_PRT=$AB_PRT -DAB_MAXFLP=false \
	-DAB_DNR=true \
	-DAB_HYOPT=false -DAB_SYMB_THRES=66.0 -DAB_NUMC_THRES=1.50 \
	-DDAHA=true -DDAHA_REAR=true \
	-DDAHA_QUANTILE=0.99 \
	-DDAHA_SYMB_THRES=$DAHA_SYMB_THRES \
	-DDAHA_NUMC_THRES=$DAHA_NUMC_THRES \
	-DAB_ACC_THRES=$AB_ACC_THRES \
	-DMIN_HT_S=8 -DDN_MIN=256 \
	-DGUD_ALPHA=0.5 -DGUD_MIN=256 -DHYB_THRES=0.95 \
	-DHASH_CONSTANT=2654435761 -DMKL_SORT=false -DMKL_ENHANCE=false \
	-DRUN_NUM=$RUN_NUM -DIN_TAMU=false -DIN_GROUPBY=false -DIN_SSJ=false \
	-DUSE_PAPI=false -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
	-DIN_DEBUG=false -DIN_EXAMINE=false \
	-DCFG_PATH=config/mc/mxc0.cfg
if ! make -j $(( $(nproc) / 16 )) "${BACKENDS[@]}"; then
	echo "ERROR: PathSim build failed; aborting before benchmarks."
	exit 1
fi
cd ..

CONC_CSV=${DIR_PATH}/${DS_NAME}_concentration.csv
echo "=== concentration (|row|, |nnz|, Gini) ==="
$PY scripts/20260208-scripts/dblp_concentration.py --selftest \
	> ${DIR_PATH}/selftest.log 2>&1 \
	&& echo "  self-test OK. ${DIR_PATH}/selftest.log" \
	|| { echo "  [FAIL] self-test — see ${DIR_PATH}/selftest.log"; exit 1; }

$PY scripts/20260208-scripts/dblp_concentration.py \
	--name ${DS_NAME} --metapath ${METAPATHS} --threads ${THREAD_NUM} --top 20 \
	--csv ${CONC_CSV} \
	--md  ${DIR_PATH}/${DS_NAME}_concentration.md \
	--plot ${FIG_PATH}/${DS_NAME}_concentration \
	> ${DIR_PATH}/${DS_NAME}_concentration.log 2>&1 \
	&& echo "  Done. ${CONC_CSV}" \
	|| { echo "  [FAIL] see ${DIR_PATH}/${DS_NAME}_concentration.log"; exit 1; }

for entry in "${QUERY_RUNS[@]}"; do
	IFS='|' read -r metapath query k <<< "$entry"
	echo "=== ${DS_NAME} ${metapath} query=${query} k=${k} ==="
	for BIN in "${BACKENDS[@]}"; do
		if [ ! -f "./bin/${BIN}" ]; then
			echo "[SKIP] ${BIN} — binary not found (MKLROOT_PATH unset for this host?)"
			continue
		fi
		LOG_FILE=${DIR_PATH}/${DS_NAME}_${BIN}_${metapath}_q${query}.log
		rm -f ${LOG_FILE}
		echo "Running ${BIN} ..."
		OMP_NUM_THREADS=${THREAD_NUM} \
		$NUMACTL --physcpubind=$CORE_RANGE \
			./bin/${BIN} ${DS_NAME} ${metapath} ${query} ${k} \
			>> ${LOG_FILE} 2>&1
		echo "  Done. Log: ${LOG_FILE}"
	done
done

bash clean.sh
cd ${CUR_DIR}

echo
$PY plot_tab3.py --md ${DIR_PATH}/${DS_NAME}_tab3.md

end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date
