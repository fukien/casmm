# mxc


date
start_time=$(date +%s)

CUR_DIR=$(pwd)

RUN_NUM=5
THREAD_NUM=$(numactl --hardware | awk '/node 0 cpus:/ {print (NF-3)/2}')
CORE_RANGE="0-31,64-95"
NUMACTL="/home/huang/workspace/numactl/numactl"



DIR_PATH=${CUR_DIR}/../../logs/20251017-logs
mkdir -p $DIR_PATH


for ddnmin in 1 2 4 8 16 32 64 128 256 512; do
	# for dataset in webbase-1M 333SP GL7d17 GL7d18 hugetric-00000 hugetric-00010 hugetric-00020; do
	for dataset in webbase-1M; do


		TAMU_1_DDNMIN_HASH_LOG=${DIR_PATH}/${dataset}_1_ddnmin${ddnmin}_hash.log
		rm $TAMU_1_DDNMIN_HASH_LOG


		THREAD_NUM=$(numactl --hardware | awk '/node 0 cpus:/ {print (NF-3)/2}')
		# THREAD_NUM=$((2 * THREAD_NUM))

		# ab_hash
		# AB_HIGH_THREAD_NUM=8
		# AB_PRT=0.95
		AB_HIGH_THREAD_NUM=16
		# AB_PRT=0.90
		AB_PRT=0.85
		# mask-6
		# DAHA_SYMB_THRES=6000000
		# DAHA_NUMC_THRES=800000
		# # mask-1
		DAHA_SYMB_THRES=11000000
		DAHA_NUMC_THRES=2500000

		# ab_hsersc
		# AB_HIGH_THREAD_NUM=22
		# AB_PRT=0.85
		# # mask-6
		# DAHA_SYMB_THRES=5000000
		# DAHA_NUMC_THRES=340000
		# mask-1
		# DAHA_SYMB_THRES=7000000
		# DAHA_NUMC_THRES=2100000

		cd ../..
		bash clean.sh
		mkdir build
		cd build
		cmake .. -DTHREAD_NUM=$THREAD_NUM -DUSE_HYPERTHREADING=true  \
			-DNUMA_MASK_VAL=1 -DUSE_WEIGHTED_INTERLEAVING=true -DUSE_INTERLEAVING=false \
			-DCSRC_WRITE_OPTIMIZED=false -DCSRC_PREPOPULATED=false -DUSE_HUGE=false \
			-DMEMSV=false -DMEMSVB=false -DMEM_MON=false \
			-DAB_HIGH_THREAD_NUM=$AB_HIGH_THREAD_NUM -DAB_SIZE=524288 -DAB_MIN_NUM=32768 \
			-DAB_LPT=false -DAB_SIMP=false -DAB_PRT=$AB_PRT -DAB_MAXFLP=false \
			-DAB_DNR=true \
			-DAB_HYOPT=false -DAB_SYMB_THRES=66.0 -DAB_NUMC_THRES=1.50 \
			-DDAHA=true -DDAHA_REAR=true \
			-DDAHA_QUANTILE=0.99 \
			-DDAHA_SYMB_THRES=$DAHA_SYMB_THRES \
			-DDAHA_NUMC_THRES=$DAHA_NUMC_THRES \
			-DDN_MIN=${ddnmin} \
			-DGUD_ALPHA=0.5 -DGUD_MIN=256 -DHYB_THRES=0.95 \
			-DHASH_CONSTANT=2654435761 -DMKL_SORT=false -DMKL_ENHANCE=false \
			-DRUN_NUM=$RUN_NUM -DIN_TAMU=true -DIN_GROUPBY=false -DIN_SSJ=false \
			-DUSE_PAPI=true -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
			-DIN_DEBUG=false -DIN_EXAMINE=false \
			-DCFG_PATH=config/mc/mxc0.cfg
		make -j $(( $(nproc) / 16 ))
		cd ..
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/dnr_hashspgemm $dataset >> $TAMU_1_DDNMIN_HASH_LOG 2>&1
		bash clean.sh
		cd ${CUR_DIR}

	done
done









end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date