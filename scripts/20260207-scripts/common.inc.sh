
RUN_NUM=3
NUMACTL="/home/huang/workspace/numactl/numactl"

RUN_LIMIT_M1=${RUN_LIMIT_M1:-600}
RUN_LIMIT_M6=${RUN_LIMIT_M6:-900}

CAP=$RUN_LIMIT_M1

CUR_DIR=$(pwd)

DIR_PATH=${CUR_DIR}/../../logs/figure14-logs
mkdir -p $DIR_PATH

DATASET_DIR=${CUR_DIR}/../../dataset/groupby-sorted


groupby_dataset_list=(
	"imdb_movie"
	"chbc_s_uni"
	"bkgn"
	"chbc_s_skew"
	"ml20m"
)

if [ -n "$ONLY" ]; then
	filtered=()
	for d in ${groupby_dataset_list[@]}; do
		if [[ " $ONLY " == *" $d "* ]]; then filtered+=("$d"); fi
	done
	groupby_dataset_list=(${filtered[@]})
fi

MASKS=${MASKS:-"1 6"}
THREADS=${THREADS:-"32 64"}


AB_ACC_THRES=1.2116
AB_HIGH_THREAD_NUM=22
AB_PRT=0.85

PHYS=$(numactl --hardware | awk '/node 0 cpus:/ {print (NF-3)/2}')


if ! command -v timeout > /dev/null 2>&1; then
	echo "ERROR: timeout(1) not found -- the RUN_LIMIT watchdog needs it."
	exit 1
fi


timed_out=()


set_mask()
{
	MASK=$1
	if [ "$MASK" = "1" ]; then
		CAP=${RUN_LIMIT:-$RUN_LIMIT_M1}
		CORE_RANGE="0-31,64-95"
		DAHA_SYMB_THRES=7000000
		DAHA_NUMC_THRES=2100000
		CFG_ARG="-DCFG_PATH=config/mc/mxc0.cfg"
		DEFAULT_WI=false
	else
		CAP=${RUN_LIMIT:-$RUN_LIMIT_M6}
		CORE_RANGE="32-63,96-127"
		DAHA_SYMB_THRES=5000000
		DAHA_NUMC_THRES=450000
		CFG_ARG=""
		DEFAULT_WI=true
	fi
}


set_threads()
{
	if [ "$1" = "64" ]; then
		THREAD_NUM=$((2 * PHYS)); HYPER=true;  SUFFIX="_hyp"
	else
		THREAD_NUM=$PHYS;         HYPER=false; SUFFIX=""
	fi
}


build()
{
	local wi=$1; shift
	local mkl_sort=$1; shift
	local targets="$@"

	echo
	echo "############################################################"
	echo "### mask=$MASK  THREAD_NUM=$THREAD_NUM  HT=$HYPER  WI=$wi  MKL_SORT=$mkl_sort"
	echo "###   targets: $targets"
	echo "############################################################"

	cd ../..
	bash clean.sh
	mkdir -p build
	cd build
	cmake .. -DTHREAD_NUM=$THREAD_NUM -DUSE_HYPERTHREADING=$HYPER  \
		-DNUMA_MASK_VAL=$MASK -DUSE_WEIGHTED_INTERLEAVING=$wi -DUSE_INTERLEAVING=false \
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
		-DDN_MIN=256 \
		-DGUD_ALPHA=0.5 -DGUD_MIN=256 -DHYB_THRES=0.95 \
		-DHASH_CONSTANT=2654435761 -DMKL_SORT=$mkl_sort -DMKL_ENHANCE=false \
		-DRUN_NUM=$RUN_NUM -DIN_TAMU=false -DIN_GROUPBY=true -DIN_SSJ=false \
		-DUSE_PAPI=false -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
		-DIN_DEBUG=false -DIN_EXAMINE=false \
		$CFG_ARG
	make -j $(( $(nproc) / 16 )) $targets
	cd ..
}


teardown()
{
	bash clean.sh
	cd ${CUR_DIR}
}


run_one()
{
	local bin=$1 dataset=$2 log=$3 tag=$4

	if [ ! -f "${DATASET_DIR}/${dataset}-sorted.org.bin" ]; then
		echo "   skip (no dataset)  ${tag}  ${dataset}"
		return
	fi
	if [ ! -x "./bin/${bin}" ]; then
		echo "   skip (no binary)   ${tag}  ${bin}  -- did the make fail?"
		return
	fi

	local t0=$(date +%s)
	timeout --signal=TERM --kill-after=20 ${CAP} \
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/${bin} $dataset >> $log 2>&1
	local rc=$?
	local dt=$(( $(date +%s) - t0 ))

	if [ $rc -eq 124 ] || [ $rc -eq 137 ] || [ $rc -eq 143 ]; then
		local why
		if [ $rc -eq 137 ] && [ $dt -lt $(( CAP + 10 )) ]; then
			why="KILLED (SIGKILL at ${dt}s, under the ${CAP}s cap -- OOM, not watchdog)"
		else
			why="TIMEOUT after ${dt}s (cap ${CAP}s, mask ${MASK})"
		fi
		echo "!! ${why}" >> $log 2>&1
		echo "   ${why}  ${tag}  ${bin}  ${dataset}  -- killing and continuing"
		force_kill $bin $dataset
		timed_out+=("${tag}:${bin}:${dataset}:${dt}s")
	else
		echo "   ok ${dt}s wall  ${tag}  ${bin}  ${dataset}"
	fi
}


force_kill()
{
	local bin=$1 dataset=$2
	local pat="bin/${bin} ${dataset}"

	pkill -TERM -f "$pat" 2> /dev/null
	sleep 5
	if pgrep -f "$pat" > /dev/null 2>&1; then
		echo "      still alive after TERM -- SIGKILL"
		pkill -KILL -f "$pat" 2> /dev/null
		sleep 3
	fi
	if pgrep -f "$pat" > /dev/null 2>&1; then
		echo "      !! STILL alive after SIGKILL (unkillable in D state?) -- pids: $(pgrep -f "$pat" | tr '\n' ' ')"
	fi
	sleep 10
}


report_timeouts()
{
	if [ ${#timed_out[@]} -gt 0 ]; then
		echo
		echo "########## timed out / killed, no result ##########"
		for x in "${timed_out[@]}"; do echo "   $x"; done
	fi
}


want_ml20m()
{
	[[ " ${groupby_dataset_list[@]} " == *" ml20m "* ]] \
		&& [ -f "${DATASET_DIR}/ml20m-sorted.org.bin" ]
}


banner()
{
	echo "physical cores on node 0: $PHYS"
	echo "masks: $MASKS   thread counts: $THREADS   RUN_NUM=$RUN_NUM"
	if [ -n "$RUN_LIMIT" ]; then
		echo "watchdog: ${RUN_LIMIT}s for every mask (RUN_LIMIT override)"
	else
		echo "watchdog: mask1=${RUN_LIMIT_M1}s  mask6=${RUN_LIMIT_M6}s  per (config, dataset, binary)"
	fi
	echo "datasets: ${groupby_dataset_list[@]}"

	local missing=()
	for dataset in ${groupby_dataset_list[@]}; do
		if [ ! -f "${DATASET_DIR}/${dataset}-sorted.org.bin" ]; then
			missing+=("$dataset")
		fi
	done
	if [ ${#missing[@]} -gt 0 ]; then
		echo "!! not present, will be skipped: ${missing[@]}"
	fi
	echo
}
