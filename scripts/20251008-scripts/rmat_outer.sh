# mxc


date
start_time=$(date +%s)

CUR_DIR=$(pwd)

RUN_NUM=5
NUMACTL="/home/huang/workspace/numactl/numactl"



DIR_PATH=${CUR_DIR}/../../logs/20251008-logs
mkdir -p $DIR_PATH





ER_6_OUTER_LOG=${DIR_PATH}/er_6_outer.log
rm $ER_6_OUTER_LOG
G500_6_OUTER_LOG=${DIR_PATH}/g500_6_outer.log
rm $G500_6_OUTER_LOG
SSCA_6_OUTER_LOG=${DIR_PATH}/ssca_6_outer.log
rm $SSCA_6_OUTER_LOG
THREAD_NUM=$(numactl --hardware | awk '/node 0 cpus:/ {print (NF-3)/2}')
CORE_RANGE="32-63"

cd ../../
export OMP_PLACES=cores
export OMP_PROC_BIND=close
for scale in 18 19 20 21; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="er_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE --weighted-interleave=1,2 ./bin/outerspgemm text \
			./dataset/rmat/${dataset}.org.mtx ./dataset/rmat/${dataset}.trs.mtx placeholder \
			$THREAD_NUM 1024 32 $dataset >> $ER_6_OUTER_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="g500_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE --weighted-interleave=1,2 ./bin/outerspgemm text \
			./dataset/rmat/${dataset}.org.mtx ./dataset/rmat/${dataset}.trs.mtx placeholder \
			$THREAD_NUM 1024 32 $dataset >> $G500_6_OUTER_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="ssca_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE --weighted-interleave=1,2 ./bin/outerspgemm text \
			./dataset/rmat/${dataset}.org.mtx ./dataset/rmat/${dataset}.trs.mtx placeholder \
			$THREAD_NUM 1024 32 $dataset >> $SSCA_6_OUTER_LOG 2>&1
	done
done
cd ${CUR_DIR}





ER_6_OUTER_HYP_LOG=${DIR_PATH}/er_6_outer_hyp.log
rm $ER_6_OUTER_HYP_LOG
G500_6_OUTER_HYP_LOG=${DIR_PATH}/g500_6_outer_hyp.log
rm $G500_6_OUTER_HYP_LOG
SSCA_6_OUTER_HYP_LOG=${DIR_PATH}/ssca_6_outer_hyp.log
rm $SSCA_6_OUTER_HYP_LOG
THREAD_NUM=$(numactl --hardware | awk '/node 0 cpus:/ {print (NF-3)/2}')
THREAD_NUM=$((2 * THREAD_NUM))
CORE_RANGE="32-63,96-127"

cd ../../
export OMP_PLACES=cores
export OMP_PROC_BIND=close
for scale in 18 19 20 21; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="er_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE --weighted-interleave=1,2 ./bin/outerspgemm text \
			./dataset/rmat/${dataset}.org.mtx ./dataset/rmat/${dataset}.trs.mtx placeholder \
			$THREAD_NUM 1024 32 $dataset >> $ER_6_OUTER_HYP_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="g500_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE --weighted-interleave=1,2 ./bin/outerspgemm text \
			./dataset/rmat/${dataset}.org.mtx ./dataset/rmat/${dataset}.trs.mtx placeholder \
			$THREAD_NUM 1024 32 $dataset >> $G500_6_OUTER_HYP_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="ssca_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE --weighted-interleave=1,2 ./bin/outerspgemm text \
			./dataset/rmat/${dataset}.org.mtx ./dataset/rmat/${dataset}.trs.mtx placeholder \
			$THREAD_NUM 1024 32 $dataset >> $SSCA_6_OUTER_HYP_LOG 2>&1
	done
done
cd ${CUR_DIR}





ER_1_OUTER_LOG=${DIR_PATH}/er_1_outer.log
rm $ER_1_OUTER_LOG
G500_1_OUTER_LOG=${DIR_PATH}/g500_1_outer.log
rm $G500_1_OUTER_LOG
SSCA_1_OUTER_LOG=${DIR_PATH}/ssca_1_outer.log
rm $SSCA_1_OUTER_LOG
THREAD_NUM=$(numactl --hardware | awk '/node 0 cpus:/ {print (NF-3)/2}')
CORE_RANGE="0-31"

cd ../../
export OMP_PLACES=cores
export OMP_PROC_BIND=close
for scale in 18 19 20 21; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="er_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE --membind=0 ./bin/outerspgemm text \
			./dataset/rmat/${dataset}.org.mtx ./dataset/rmat/${dataset}.trs.mtx placeholder \
			$THREAD_NUM 1024 32 $dataset >> $ER_1_OUTER_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="g500_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE --membind=0 ./bin/outerspgemm text \
			./dataset/rmat/${dataset}.org.mtx ./dataset/rmat/${dataset}.trs.mtx placeholder \
			$THREAD_NUM 1024 32 $dataset >> $G500_1_OUTER_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="ssca_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE --membind=0 ./bin/outerspgemm text \
			./dataset/rmat/${dataset}.org.mtx ./dataset/rmat/${dataset}.trs.mtx placeholder \
			$THREAD_NUM 1024 32 $dataset >> $SSCA_1_OUTER_LOG 2>&1
	done
done
cd ${CUR_DIR}





ER_1_OUTER_HYP_LOG=${DIR_PATH}/er_1_outer_hyp.log
rm $ER_1_OUTER_HYP_LOG
G500_1_OUTER_HYP_LOG=${DIR_PATH}/g500_1_outer_hyp.log
rm $G500_1_OUTER_HYP_LOG
SSCA_1_OUTER_HYP_LOG=${DIR_PATH}/ssca_1_outer_hyp.log
rm $SSCA_1_OUTER_HYP_LOG
THREAD_NUM=$(numactl --hardware | awk '/node 0 cpus:/ {print (NF-3)/2}')
THREAD_NUM=$((2 * THREAD_NUM))
CORE_RANGE="0-31,64-95"

cd ../../
export OMP_PLACES=cores
export OMP_PROC_BIND=close
for scale in 18 19 20 21; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="er_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE --membind=0 ./bin/outerspgemm text \
			./dataset/rmat/${dataset}.org.mtx ./dataset/rmat/${dataset}.trs.mtx placeholder \
			$THREAD_NUM 1024 32 $dataset >> $ER_1_OUTER_HYP_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="g500_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE --membind=0 ./bin/outerspgemm text \
			./dataset/rmat/${dataset}.org.mtx ./dataset/rmat/${dataset}.trs.mtx placeholder \
			$THREAD_NUM 1024 32 $dataset >> $G500_1_OUTER_HYP_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="ssca_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE --membind=0 ./bin/outerspgemm text \
			./dataset/rmat/${dataset}.org.mtx ./dataset/rmat/${dataset}.trs.mtx placeholder \
			$THREAD_NUM 1024 32 $dataset >> $SSCA_1_OUTER_HYP_LOG 2>&1
	done
done
cd ${CUR_DIR}









end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date