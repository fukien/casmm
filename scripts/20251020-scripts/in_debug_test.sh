# mxc


date
start_time=$(date +%s)

CUR_DIR=$(pwd)

RUN_NUM=2
THREAD_NUM=32
CORE_RANGE="32-63,96-127"
NUMACTL="/home/huang/workspace/numactl/numactl"

DIR_PATH=${CUR_DIR}/../../logs/20251020-logs
mkdir -p $DIR_PATH


ER_6_HASH_LOG=${DIR_PATH}/er_6_hash.log
rm $ER_6_HASH_LOG
ER_6_HSR_LOG=${DIR_PATH}/er_6_hsr.log
rm $ER_6_HSR_LOG
G500_6_HASH_LOG=${DIR_PATH}/g500_6_hash.log
rm $G500_6_HASH_LOG
G500_6_HSR_LOG=${DIR_PATH}/g500_6_hsr.log
rm $G500_6_HSR_LOG


cd ../..
bash clean.sh
mkdir build
cd build
cmake .. -DCSRC_WRITE_OPTIMIZED=false -DCSRC_PREPOPULATED=false -DUSE_HUGE=false -DTHREAD_NUM=$THREAD_NUM -DUSE_HYPERTHREADING=true \
	-DNUMA_MASK_VAL=6 -DUSE_WEIGHTED_INTERLEAVING=true -DUSE_INTERLEAVING=false -DHASH_CONSTANT=2654435761 \
	-DCF_MARK_0=1.7 -DCF_MARK_1=2.2 -DMKL_SORT=true -DRUN_NUM=$RUN_NUM -DIN_TAMU=false -DIN_GROUPBY=false -DIN_SSJ=false \
	-DUSE_PAPI=false -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false -DIN_DEBUG=true -DIN_EXAMINE=true
make -j 16
cd ..
for scale in 18 19 20 21; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="er_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/hashspgemm $dataset >> $ER_6_HASH_LOG 2>&1
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/hserscspgemm $dataset >> $ER_6_HSR_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="g500_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/hashspgemm $dataset >> $G500_6_HASH_LOG 2>&1
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/hserscspgemm $dataset >> $G500_6_HSR_LOG 2>&1
	done
done
bash clean.sh
cd ${CUR_DIR}


cd ../../
bash clean.sh

end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date