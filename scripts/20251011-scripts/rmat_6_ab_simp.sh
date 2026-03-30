# mxc


date
start_time=$(date +%s)


RUN_NUM=5
NUMACTL="/home/huang/workspace/numactl/numactl"

CUR_DIR=$(pwd)


DIR_PATH=${CUR_DIR}/../../logs/20251011-logs
mkdir -p $DIR_PATH




AB_PRT=0.85
DAHA_SYMB_THRES=5000000
DAHA_NUMC_THRES=340000
AB_ACC_THRES=2.116



CORE_RANGE="32-63,96-127"
THREAD_NUM=$(numactl --hardware | awk '/node 0 cpus:/ {print (NF-3)/2}')
THREAD_NUM=$((2 * THREAD_NUM))
ER_6_AB_HYBACC_DAHA_SIMP_LOG=${DIR_PATH}/er_6_ab_hybacc_daha_simp.log
rm $ER_6_AB_HYBACC_DAHA_SIMP_LOG
G500_6_AB_HYBACC_DAHA_SIMP_LOG=${DIR_PATH}/g500_6_ab_hybacc_daha_simp.log
rm $G500_6_AB_HYBACC_DAHA_SIMP_LOG
SSCA_6_AB_HYBACC_DAHA_SIMP_LOG=${DIR_PATH}/ssca_6_ab_hybacc_daha_simp.log
rm $SSCA_6_AB_HYBACC_DAHA_SIMP_LOG

AB_HIGH_THREAD_NUM=15

cd ../..
bash clean.sh
mkdir build
cd build
cmake .. -DTHREAD_NUM=$THREAD_NUM -DUSE_HYPERTHREADING=true  \
	-DNUMA_MASK_VAL=6 -DUSE_WEIGHTED_INTERLEAVING=true -DUSE_INTERLEAVING=false \
	-DCSRC_WRITE_OPTIMIZED=false -DCSRC_PREPOPULATED=false -DUSE_HUGE=false \
	-DMEMSV=false -DMEMSVB=false -DMEM_MON=false \
	-DAB_HIGH_THREAD_NUM=$AB_HIGH_THREAD_NUM -DAB_SIZE=524288 -DAB_MIN_NUM=32768 \
	-DAB_LPT=false -DAB_SIMP=true -DAB_PRT=$AB_PRT -DAB_MAXFLP=false \
	-DAB_DNR=true \
	-DAB_HYOPT=false -DAB_SYMB_THRES=66.0 -DAB_NUMC_THRES=1.50 \
	-DDAHA=true -DDAHA_REAR=true \
	-DDAHA_QUANTILE=0.99 \
	-DDAHA_SYMB_THRES=$DAHA_SYMB_THRES \
	-DDAHA_NUMC_THRES=$DAHA_NUMC_THRES \
	-DDAHA_SPMV_THRES=$DAHA_SPMV_THRES \
	-DDAHA_SPMM_THRES=$DAHA_SPMM_THRES \
	-DAB_ACC_THRES=$AB_ACC_THRES \
	-DDN_MIN=256 \
	-DGUD_ALPHA=0.5 -DGUD_MIN=256 -DHYB_THRES=0.95 \
	-DHASH_CONSTANT=2654435761 	-DSPMM_B_K=512 \
	-DMKL_SORT=false -DMKL_ENHANCE=false \
	-DRUN_NUM=$RUN_NUM -DIN_TAMU=false -DIN_GROUPBY=false -DIN_SSJ=false \
	-DUSE_PAPI=false -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
	-DIN_DEBUG=false -DIN_EXAMINE=false # \
	# -DCFG_PATH=config/mc/mxc0.cfg
make -j $(( $(nproc) / 16 ))
cd ..
for scale in 18 19 20 21; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="er_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hybaccspgemm $dataset >> $ER_6_AB_HYBACC_DAHA_SIMP_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="g500_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hybaccspgemm $dataset >> $G500_6_AB_HYBACC_DAHA_SIMP_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="ssca_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hybaccspgemm $dataset >> $SSCA_6_AB_HYBACC_DAHA_SIMP_LOG 2>&1
	done
done
bash clean.sh
cd ${CUR_DIR}









CORE_RANGE="32-63,96-127"
THREAD_NUM=$(numactl --hardware | awk '/node 0 cpus:/ {print (NF-3)/2}')
THREAD_NUM=$((2 * THREAD_NUM))
ER_6_AB_HASH_DAHA_SIMP_LOG=${DIR_PATH}/er_6_ab_hash_daha_simp.log
rm $ER_6_AB_HASH_DAHA_SIMP_LOG
G500_6_AB_HASH_DAHA_SIMP_LOG=${DIR_PATH}/g500_6_ab_hash_daha_simp.log
rm $G500_6_AB_HASH_DAHA_SIMP_LOG
SSCA_6_AB_HASH_DAHA_SIMP_LOG=${DIR_PATH}/ssca_6_ab_hash_daha_simp.log
rm $SSCA_6_AB_HASH_DAHA_SIMP_LOG

AB_HIGH_THREAD_NUM=12

cd ../..
bash clean.sh
mkdir build
cd build
cmake .. -DTHREAD_NUM=$THREAD_NUM -DUSE_HYPERTHREADING=true  \
	-DNUMA_MASK_VAL=6 -DUSE_WEIGHTED_INTERLEAVING=true -DUSE_INTERLEAVING=false \
	-DCSRC_WRITE_OPTIMIZED=false -DCSRC_PREPOPULATED=false -DUSE_HUGE=false \
	-DMEMSV=false -DMEMSVB=false -DMEM_MON=false \
	-DAB_HIGH_THREAD_NUM=$AB_HIGH_THREAD_NUM -DAB_SIZE=524288 -DAB_MIN_NUM=32768 \
	-DAB_LPT=false -DAB_SIMP=true -DAB_PRT=$AB_PRT -DAB_MAXFLP=false \
	-DAB_DNR=true \
	-DAB_HYOPT=false -DAB_SYMB_THRES=66.0 -DAB_NUMC_THRES=1.50 \
	-DDAHA=true -DDAHA_REAR=true \
	-DDAHA_QUANTILE=0.99 \
	-DDAHA_SYMB_THRES=$DAHA_SYMB_THRES \
	-DDAHA_NUMC_THRES=$DAHA_NUMC_THRES \
	-DDAHA_SPMV_THRES=$DAHA_SPMV_THRES \
	-DDAHA_SPMM_THRES=$DAHA_SPMM_THRES \
	-DAB_ACC_THRES=$AB_ACC_THRES \
	-DDN_MIN=256 \
	-DGUD_ALPHA=0.5 -DGUD_MIN=256 -DHYB_THRES=0.95 \
	-DHASH_CONSTANT=2654435761 	-DSPMM_B_K=512 \
	-DMKL_SORT=false -DMKL_ENHANCE=false \
	-DRUN_NUM=$RUN_NUM -DIN_TAMU=false -DIN_GROUPBY=false -DIN_SSJ=false \
	-DUSE_PAPI=false -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
	-DIN_DEBUG=false -DIN_EXAMINE=false # \
	# -DCFG_PATH=config/mc/mxc0.cfg
make -j $(( $(nproc) / 16 ))
cd ..
for scale in 18 19 20 21; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="er_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $ER_6_AB_HASH_DAHA_SIMP_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="g500_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $G500_6_AB_HASH_DAHA_SIMP_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="ssca_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $SSCA_6_AB_HASH_DAHA_SIMP_LOG 2>&1
	done
done
bash clean.sh
cd ${CUR_DIR}







CORE_RANGE="32-63,96-127"
THREAD_NUM=$(numactl --hardware | awk '/node 0 cpus:/ {print (NF-3)/2}')
THREAD_NUM=$((2 * THREAD_NUM))
ER_6_AB_HSERSC_DAHA_SIMP_LOG=${DIR_PATH}/er_6_ab_hsersc_daha_simp.log
rm $ER_6_AB_HSERSC_DAHA_SIMP_LOG
G500_6_AB_HSERSC_DAHA_SIMP_LOG=${DIR_PATH}/g500_6_ab_hsersc_daha_simp.log
rm $G500_6_AB_HSERSC_DAHA_SIMP_LOG
SSCA_6_AB_HSERSC_DAHA_SIMP_LOG=${DIR_PATH}/ssca_6_ab_hsersc_daha_simp.log
rm $SSCA_6_AB_HSERSC_DAHA_SIMP_LOG

AB_HIGH_THREAD_NUM=24

cd ../..
bash clean.sh
mkdir build
cd build
cmake .. -DTHREAD_NUM=$THREAD_NUM -DUSE_HYPERTHREADING=true  \
	-DNUMA_MASK_VAL=6 -DUSE_WEIGHTED_INTERLEAVING=true -DUSE_INTERLEAVING=false \
	-DCSRC_WRITE_OPTIMIZED=false -DCSRC_PREPOPULATED=false -DUSE_HUGE=false \
	-DMEMSV=false -DMEMSVB=false -DMEM_MON=false \
	-DAB_HIGH_THREAD_NUM=$AB_HIGH_THREAD_NUM -DAB_SIZE=524288 -DAB_MIN_NUM=32768 \
	-DAB_LPT=false -DAB_SIMP=true -DAB_PRT=$AB_PRT -DAB_MAXFLP=false \
	-DAB_DNR=true \
	-DAB_HYOPT=false -DAB_SYMB_THRES=66.0 -DAB_NUMC_THRES=1.50 \
	-DDAHA=true -DDAHA_REAR=true \
	-DDAHA_QUANTILE=0.99 \
	-DDAHA_SYMB_THRES=$DAHA_SYMB_THRES \
	-DDAHA_NUMC_THRES=$DAHA_NUMC_THRES \
	-DDAHA_SPMV_THRES=$DAHA_SPMV_THRES \
	-DDAHA_SPMM_THRES=$DAHA_SPMM_THRES \
	-DAB_ACC_THRES=$AB_ACC_THRES \
	-DDN_MIN=256 \
	-DGUD_ALPHA=0.5 -DGUD_MIN=256 -DHYB_THRES=0.95 \
	-DHASH_CONSTANT=2654435761 	-DSPMM_B_K=512 \
	-DMKL_SORT=false -DMKL_ENHANCE=false \
	-DRUN_NUM=$RUN_NUM -DIN_TAMU=false -DIN_GROUPBY=false -DIN_SSJ=false \
	-DUSE_PAPI=false -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
	-DIN_DEBUG=false -DIN_EXAMINE=false # \
	# -DCFG_PATH=config/mc/mxc0.cfg
make -j $(( $(nproc) / 16 ))
cd ..
for scale in 18 19 20 21; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="er_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hserscspgemm $dataset >> $ER_6_AB_HSERSC_DAHA_SIMP_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="g500_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hserscspgemm $dataset >> $G500_6_AB_HSERSC_DAHA_SIMP_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="ssca_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hserscspgemm $dataset >> $SSCA_6_AB_HSERSC_DAHA_SIMP_LOG 2>&1
	done
done
bash clean.sh
cd ${CUR_DIR}









end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date