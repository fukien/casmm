# mxc


date
start_time=$(date +%s)


RUN_NUM=4
NUMACTL="/home/huang/workspace/numactl/numactl"

CUR_DIR=$(pwd)


DIR_PATH=${CUR_DIR}/../../logs/20251019-logs
mkdir -p $DIR_PATH









CORE_RANGE="32-63,96-127"
THREAD_NUM=$(numactl --hardware | awk '/node 0 cpus:/ {print (NF-3)/2}')
THREAD_NUM=$((2 * THREAD_NUM))
AB_HIGH_THREAD_NUM=16
AB_PRT=0.90
ER_6_AB_HASH_HYP_LOG=${DIR_PATH}/er_6_ab_hash_hyp.log
rm $ER_6_AB_HASH_HYP_LOG
G500_6_AB_HASH_HYP_LOG=${DIR_PATH}/g500_6_ab_hash_hyp.log
rm $G500_6_AB_HASH_HYP_LOG
SSCA_6_AB_HASH_HYP_LOG=${DIR_PATH}/ssca_6_ab_hash_hyp.log
rm $SSCA_6_AB_HASH_HYP_LOG
cd ../..
bash clean.sh
mkdir build
cd build
cmake .. -DTHREAD_NUM=$THREAD_NUM -DUSE_HYPERTHREADING=true  \
	-DNUMA_MASK_VAL=6 -DUSE_WEIGHTED_INTERLEAVING=true -DUSE_INTERLEAVING=false \
	-DCSRC_WRITE_OPTIMIZED=false -DCSRC_PREPOPULATED=false -DUSE_HUGE=false \
	-DMEMSV=false -DMEMSVB=false -DMEM_MON=false \
	-DAB_HIGH_THREAD_NUM=$AB_HIGH_THREAD_NUM -DAB_SIZE=524288 -DAB_MIN_NUM=32768 \
	-DAB_SIMP=false -DAB_PRT=$AB_PRT -DAB_MAXFLP=false \
	-DAB_DNR=true \
	-DAB_HYOPT=false -DAB_SYMB_THRES=66.0 -DAB_NUMC_THRES=1.50 \
	-DDN_MIN=256 \
	-DGUD_ALPHA=0.5 -DGUD_MIN=256 -DHYB_THRES=0.95 \
	-DHASH_CONSTANT=2654435761 -DMKL_SORT=false \
	-DRUN_NUM=$RUN_NUM -DIN_TAMU=false -DIN_GROUPBY=false -DIN_SSJ=false \
	-DUSE_PAPI=false -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
	-DIN_DEBUG=false -DIN_EXAMINE=false
make -j $(( $(nproc) / 16 ))
cd ..
for scale in 18 19 20 21; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="er_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $ER_6_AB_HASH_HYP_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="g500_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $G500_6_AB_HASH_HYP_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="ssca_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $SSCA_6_AB_HASH_HYP_LOG 2>&1
	done
done
bash clean.sh
cd ${CUR_DIR}






CORE_RANGE="0-31,64-95"
THREAD_NUM=$(numactl --hardware | awk '/node 0 cpus:/ {print (NF-3)/2}')
THREAD_NUM=$((2 * THREAD_NUM))
AB_HIGH_THREAD_NUM=16
AB_PRT=0.90
ER_1_AB_HASH_HYP_LOG=${DIR_PATH}/er_1_ab_hash_hyp.log
rm $ER_1_AB_HASH_HYP_LOG
G500_1_AB_HASH_HYP_LOG=${DIR_PATH}/g500_1_ab_hash_hyp.log
rm $G500_1_AB_HASH_HYP_LOG
SSCA_1_AB_HASH_HYP_LOG=${DIR_PATH}/ssca_1_ab_hash_hyp.log
rm $SSCA_1_AB_HASH_HYP_LOG
cd ../..
bash clean.sh
mkdir build
cd build
cmake .. -DTHREAD_NUM=$THREAD_NUM -DUSE_HYPERTHREADING=true  \
	-DNUMA_MASK_VAL=1 -DUSE_WEIGHTED_INTERLEAVING=false -DUSE_INTERLEAVING=false \
	-DCSRC_WRITE_OPTIMIZED=false -DCSRC_PREPOPULATED=false -DUSE_HUGE=false \
	-DMEMSV=false -DMEMSVB=false -DMEM_MON=false \
	-DAB_HIGH_THREAD_NUM=$AB_HIGH_THREAD_NUM -DAB_SIZE=524288 -DAB_MIN_NUM=32768 \
	-DAB_SIMP=false -DAB_PRT=$AB_PRT -DAB_MAXFLP=false \
	-DAB_DNR=true \
	-DAB_HYOPT=false -DAB_SYMB_THRES=66.0 -DAB_NUMC_THRES=1.50 \
	-DDN_MIN=256 \
	-DGUD_ALPHA=0.5 -DGUD_MIN=256 -DHYB_THRES=0.95 \
	-DHASH_CONSTANT=2654435761 -DMKL_SORT=false \
	-DRUN_NUM=$RUN_NUM -DIN_TAMU=false -DIN_GROUPBY=false -DIN_SSJ=false \
	-DUSE_PAPI=false -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
	-DIN_DEBUG=false -DIN_EXAMINE=false \
	-DCFG_PATH=config/mc/mxc0.cfg
make -j $(( $(nproc) / 16 ))
cd ..
for scale in 18 19 20 21; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="er_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $ER_1_AB_HASH_HYP_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="g500_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $G500_1_AB_HASH_HYP_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="ssca_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $SSCA_1_AB_HASH_HYP_LOG 2>&1
	done
done
bash clean.sh
cd ${CUR_DIR}










end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date