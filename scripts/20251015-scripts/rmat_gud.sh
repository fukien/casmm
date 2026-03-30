# mxc


date
start_time=$(date +%s)

CUR_DIR=$(pwd)

RUN_NUM=5
THREAD_NUM=$(numactl --hardware | awk '/node 0 cpus:/ {print (NF-3)/2}')
CORE_RANGE="32-63,96-127"
NUMACTL="/home/huang/workspace/numactl/numactl"



DIR_PATH=${CUR_DIR}/../../logs/20251015-logs
mkdir -p $DIR_PATH




gudmin=256




ER_6_GUDMIN_LOG=${DIR_PATH}/er_6_gudmin${gudmin}.log
rm $ER_6_GUDMIN_LOG
G500_6_GUDMIN_LOG=${DIR_PATH}/g500_6_gudmin${gudmin}.log
rm $G500_6_GUDMIN_LOG
SSCA_6_GUDMIN_LOG=${DIR_PATH}/ssca_6_gudmin${gudmin}.log
rm $SSCA_6_GUDMIN_LOG
THREAD_NUM=$(numactl --hardware | awk '/node 0 cpus:/ {print (NF-3)/2}')
CORE_RANGE="32-63,96-127"
cd ../..
bash clean.sh
mkdir build
cd build
cmake .. -DTHREAD_NUM=$THREAD_NUM -DUSE_HYPERTHREADING=true  \
	-DNUMA_MASK_VAL=6 -DUSE_WEIGHTED_INTERLEAVING=true -DUSE_INTERLEAVING=false \
	-DCSRC_WRITE_OPTIMIZED=false -DCSRC_PREPOPULATED=false -DUSE_HUGE=false \
	-DMEMSV=false -DMEMSVB=false -DMEM_MON=false \
	-DDN_MIN=256 \
	-DGUD_ALPHA=0.5 -DGUD_MIN=${gudmin} -DHYB_THRES=0.95 \
	-DHASH_CONSTANT=2654435761 -DMKL_SORT=false \
	-DRUN_NUM=$RUN_NUM -DIN_TAMU=false -DIN_GROUPBY=false -DIN_SSJ=false \
	-DUSE_PAPI=false -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
	-DIN_DEBUG=false -DIN_EXAMINE=false
make -j $(( $(nproc) / 16 ))
cd ..
for scale in 18 19 20 21; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="er_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/microbench/gud_hashspgemm $dataset >> $ER_6_GUDMIN_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="g500_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/microbench/gud_hashspgemm $dataset >> $G500_6_GUDMIN_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="ssca_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/microbench/gud_hashspgemm $dataset >> $SSCA_6_GUDMIN_LOG 2>&1
	done
done
bash clean.sh
cd ${CUR_DIR}








ER_1_GUDMIN_LOG=${DIR_PATH}/er_1_gudmin${gudmin}.log
rm $ER_1_GUDMIN_LOG
G500_1_GUDMIN_LOG=${DIR_PATH}/g500_1_gudmin${gudmin}.log
rm $G500_1_GUDMIN_LOG
SSCA_1_GUDMIN_LOG=${DIR_PATH}/ssca_1_gudmin${gudmin}.log
rm $SSCA_1_GUDMIN_LOG
THREAD_NUM=$(numactl --hardware | awk '/node 0 cpus:/ {print (NF-3)/2}')
CORE_RANGE="0-31,64-95"
cd ../..
bash clean.sh
mkdir build
cd build
cmake .. -DTHREAD_NUM=$THREAD_NUM -DUSE_HYPERTHREADING=true  \
	-DNUMA_MASK_VAL=1 -DUSE_WEIGHTED_INTERLEAVING=false -DUSE_INTERLEAVING=false \
	-DCSRC_WRITE_OPTIMIZED=false -DCSRC_PREPOPULATED=false -DUSE_HUGE=false \
	-DMEMSV=false -DMEMSVB=false -DMEM_MON=false \
	-DDN_MIN=256 \
	-DGUD_ALPHA=0.5 -DGUD_MIN=${gudmin} -DHYB_THRES=0.95 \
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
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/microbench/gud_hashspgemm $dataset >> $ER_1_GUDMIN_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="g500_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/microbench/gud_hashspgemm $dataset >> $G500_1_GUDMIN_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="ssca_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/microbench/gud_hashspgemm $dataset >> $SSCA_1_GUDMIN_LOG 2>&1
	done
done
bash clean.sh
cd ${CUR_DIR}





end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date