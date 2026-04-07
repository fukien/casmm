# mxc


date
start_time=$(date +%s)


RUN_NUM=5
NUMACTL="/home/huang/workspace/numactl/numactl"

CUR_DIR=$(pwd)


DIR_PATH=${CUR_DIR}/../../logs/20260206-logs
mkdir -p $DIR_PATH






groupby_dataset_list=(
	"lsfm"
	"gdbk"
	"joke3"
	"joke1"
	"joke2"
	"lfsoi"
	"rd23"
	"isf18"
	"ml20m"
	"sac18"
	"bkgn"
	"chbc_s_uni"
	"chbc_s_skew"
)






AB_ACC_THRES=1.2116






CORE_RANGE="0-31,64-95"
THREAD_NUM=$(numactl --hardware | awk '/node 0 cpus:/ {print (NF-3)/2}')
THREAD_NUM=$((2 * THREAD_NUM))
AB_HIGH_THREAD_NUM=22
AB_PRT=0.85
DAHA_SYMB_THRES=7000000
DAHA_NUMC_THRES=2100000
# AB_ACC_THRES=2.00
GROUPBY_1_HASH_HYP_LOG=${DIR_PATH}/groupby_1_hash_hyp.log
rm $GROUPBY_1_HASH_HYP_LOG
GROUPBY_1_MKL_HYP_LOG=${DIR_PATH}/groupby_1_mkl_hyp.log
rm $GROUPBY_1_MKL_HYP_LOG
GROUPBY_1_PHJ_RDX_BC_HYP_LOG=${DIR_PATH}/groupby_1_phj_rdx_bc_hyp.log
rm $GROUPBY_1_PHJ_RDX_BC_HYP_LOG
GROUPBY_1_PHJ_RDX_BC_AGG_HYP_LOG=${DIR_PATH}/groupby_1_phj_rdx_bc_agg_hyp.log
rm $GROUPBY_1_PHJ_RDX_BC_AGG_HYP_LOG
GROUPBY_1_NPHJ_SC_HYP_LOG=${DIR_PATH}/groupby_1_nphj_sc_hyp.log
rm $GROUPBY_1_NPHJ_SC_HYP_LOG
GROUPBY_1_NPHJ_SC_AGGOTFOA_HYP_LOG=${DIR_PATH}/groupby_1_nphj_sc_aggotfoa_hyp.log
rm $GROUPBY_1_NPHJ_SC_AGGOTFOA_HYP_LOG
GROUPBY_1_DIM3_HYP_LOG=${DIR_PATH}/groupby_1_dim3_hyp.log
rm $GROUPBY_1_DIM3_HYP_LOG
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
	-DDAHA=true -DDAHA_REAR=true \
	-DDAHA_QUANTILE=0.99 \
	-DDAHA_SYMB_THRES=$DAHA_SYMB_THRES \
	-DDAHA_NUMC_THRES=$DAHA_NUMC_THRES \
	-DAB_ACC_THRES=$AB_ACC_THRES \
	-DDN_MIN=256 \
	-DGUD_ALPHA=0.5 -DGUD_MIN=256 -DHYB_THRES=0.95 \
	-DHASH_CONSTANT=2654435761 -DMKL_SORT=false -DMKL_ENHANCE=false \
	-DRUN_NUM=$RUN_NUM -DIN_TAMU=false -DIN_GROUPBY=true -DIN_SSJ=false \
	-DUSE_PAPI=false -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
	-DIN_DEBUG=false -DIN_EXAMINE=false \
	-DCFG_PATH=config/mc/mxc0.cfg
make -j $(( $(nproc) / 16 ))
cd ..
for dataset in ${groupby_dataset_list[@]}; do
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/hashspgemm $dataset >> $GROUPBY_1_HASH_HYP_LOG 2>&1
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/mkl_dcsrmultcsr $dataset >> $GROUPBY_1_MKL_HYP_LOG 2>&1
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/phj_rdx_bc $dataset >> $GROUPBY_1_PHJ_RDX_BC_HYP_LOG 2>&1
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/phj_rdx_bc_agg $dataset >> $GROUPBY_1_PHJ_RDX_BC_AGG_HYP_LOG 2>&1
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/nphj_sc $dataset >> $GROUPBY_1_NPHJ_SC_HYP_LOG 2>&1
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/nphj_sc_aggotfoa $dataset >> $GROUPBY_1_NPHJ_SC_AGGOTFOA_HYP_LOG 2>&1
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/dim3 $dataset >> $GROUPBY_1_DIM3_HYP_LOG 2>&1
done
bash clean.sh
cd ${CUR_DIR}



cd ../..
bash clean.sh
mkdir build
cd build
cmake .. -DTHREAD_NUM=$THREAD_NUM -DUSE_HYPERTHREADING=true  \
	-DNUMA_MASK_VAL=1 -DUSE_WEIGHTED_INTERLEAVING=true -DUSE_INTERLEAVING=false \
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
	-DHASH_CONSTANT=2654435761 -DMKL_SORT=false -DMKL_ENHANCE=false \
	-DRUN_NUM=$RUN_NUM -DIN_TAMU=false -DIN_GROUPBY=true -DIN_SSJ=false \
	-DUSE_PAPI=false -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
	-DIN_DEBUG=false -DIN_EXAMINE=false \
	-DCFG_PATH=config/mc/mxc0.cfg
make -j $(( $(nproc) / 16 ))
cd ..
$NUMACTL --physcpubind=$CORE_RANGE ./bin/hashspgemm "ml20m" >> $GROUPBY_1_HASH_HYP_LOG 2>&1
$NUMACTL --physcpubind=$CORE_RANGE ./bin/mkl_dcsrmultcsr "ml20m" >> $GROUPBY_1_MKL_HYP_LOG 2>&1
$NUMACTL --physcpubind=$CORE_RANGE ./bin/phj_rdx_bc "ml20m" >> $GROUPBY_1_PHJ_RDX_BC_HYP_LOG 2>&1
$NUMACTL --physcpubind=$CORE_RANGE ./bin/phj_rdx_bc_agg "ml20m" >> $GROUPBY_1_PHJ_RDX_BC_AGG_HYP_LOG 2>&1
$NUMACTL --physcpubind=$CORE_RANGE ./bin/nphj_sc "ml20m" >> $GROUPBY_1_NPHJ_SC_HYP_LOG 2>&1
$NUMACTL --physcpubind=$CORE_RANGE ./bin/nphj_sc_aggotfoa "ml20m" >> $GROUPBY_1_NPHJ_SC_AGGOTFOA_HYP_LOG 2>&1
$NUMACTL --physcpubind=$CORE_RANGE ./bin/dim3 "ml20m" >> $GROUPBY_1_DIM3_HYP_LOG 2>&1
bash clean.sh
cd ${CUR_DIR}












CORE_RANGE="0-31,64-95"
THREAD_NUM=$(numactl --hardware | awk '/node 0 cpus:/ {print (NF-3)/2}')
# THREAD_NUM=$((2 * THREAD_NUM))
AB_HIGH_THREAD_NUM=22
AB_PRT=0.85
DAHA_SYMB_THRES=7000000
DAHA_NUMC_THRES=2100000
# AB_ACC_THRES=2.00
GROUPBY_1_HASH_LOG=${DIR_PATH}/groupby_1_hash.log
rm $GROUPBY_1_HASH_LOG
GROUPBY_1_MKL_LOG=${DIR_PATH}/groupby_1_mkl.log
rm $GROUPBY_1_MKL_LOG
GROUPBY_1_PHJ_RDX_BC_LOG=${DIR_PATH}/groupby_1_phj_rdx_bc.log
rm $GROUPBY_1_PHJ_RDX_BC_LOG
GROUPBY_1_PHJ_RDX_BC_AGG_LOG=${DIR_PATH}/groupby_1_phj_rdx_bc_agg.log
rm $GROUPBY_1_PHJ_RDX_BC_AGG_LOG
GROUPBY_1_NPHJ_SC_LOG=${DIR_PATH}/groupby_1_nphj_sc.log
rm $GROUPBY_1_NPHJ_SC_LOG
GROUPBY_1_NPHJ_SC_AGGOTFOA_LOG=${DIR_PATH}/groupby_1_nphj_sc_aggotfoa.log
rm $GROUPBY_1_NPHJ_SC_AGGOTFOA_LOG
GROUPBY_1_DIM3_LOG=${DIR_PATH}/groupby_1_dim3.log
rm $GROUPBY_1_DIM3_LOG
cd ../..
bash clean.sh
mkdir build
cd build
cmake .. -DTHREAD_NUM=$THREAD_NUM -DUSE_HYPERTHREADING=false  \
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
	-DDN_MIN=256 \
	-DGUD_ALPHA=0.5 -DGUD_MIN=256 -DHYB_THRES=0.95 \
	-DHASH_CONSTANT=2654435761 -DMKL_SORT=false -DMKL_ENHANCE=false \
	-DRUN_NUM=$RUN_NUM -DIN_TAMU=false -DIN_GROUPBY=true -DIN_SSJ=false \
	-DUSE_PAPI=false -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
	-DIN_DEBUG=false -DIN_EXAMINE=false \
	-DCFG_PATH=config/mc/mxc0.cfg
make -j $(( $(nproc) / 16 ))
cd ..
for dataset in ${groupby_dataset_list[@]}; do
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/hashspgemm $dataset >> $GROUPBY_1_HASH_LOG 2>&1
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/mkl_dcsrmultcsr $dataset >> $GROUPBY_1_MKL_LOG 2>&1
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/phj_rdx_bc $dataset >> $GROUPBY_1_PHJ_RDX_BC_LOG 2>&1
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/phj_rdx_bc_agg $dataset >> $GROUPBY_1_PHJ_RDX_BC_AGG_LOG 2>&1
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/nphj_sc $dataset >> $GROUPBY_1_NPHJ_SC_LOG 2>&1
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/nphj_sc_aggotfoa $dataset >> $GROUPBY_1_NPHJ_SC_AGGOTFOA_LOG 2>&1
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/dim3 $dataset >> $GROUPBY_1_DIM3_LOG 2>&1
done
bash clean.sh
cd ${CUR_DIR}




cd ../..
bash clean.sh
mkdir build
cd build
cmake .. -DTHREAD_NUM=$THREAD_NUM -DUSE_HYPERTHREADING=false  \
	-DNUMA_MASK_VAL=1 -DUSE_WEIGHTED_INTERLEAVING=true -DUSE_INTERLEAVING=false \
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
	-DHASH_CONSTANT=2654435761 -DMKL_SORT=false -DMKL_ENHANCE=false \
	-DRUN_NUM=$RUN_NUM -DIN_TAMU=false -DIN_GROUPBY=true -DIN_SSJ=false \
	-DUSE_PAPI=false -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
	-DIN_DEBUG=false -DIN_EXAMINE=false \
	-DCFG_PATH=config/mc/mxc0.cfg
make -j $(( $(nproc) / 16 ))
cd ..
$NUMACTL --physcpubind=$CORE_RANGE ./bin/hashspgemm "ml20m" >> $GROUPBY_1_HASH_LOG 2>&1
$NUMACTL --physcpubind=$CORE_RANGE ./bin/mkl_dcsrmultcsr "ml20m" >> $GROUPBY_1_MKL_LOG 2>&1
$NUMACTL --physcpubind=$CORE_RANGE ./bin/phj_rdx_bc "ml20m" >> $GROUPBY_1_PHJ_RDX_BC_LOG 2>&1
$NUMACTL --physcpubind=$CORE_RANGE ./bin/phj_rdx_bc_agg "ml20m" >> $GROUPBY_1_PHJ_RDX_BC_AGG_LOG 2>&1
$NUMACTL --physcpubind=$CORE_RANGE ./bin/nphj_sc "ml20m" >> $GROUPBY_1_NPHJ_SC_LOG 2>&1
$NUMACTL --physcpubind=$CORE_RANGE ./bin/nphj_sc_aggotfoa "ml20m" >> $GROUPBY_1_NPHJ_SC_AGGOTFOA_LOG 2>&1
$NUMACTL --physcpubind=$CORE_RANGE ./bin/dim3 "ml20m" >> $GROUPBY_1_DIM3_LOG 2>&1
bash clean.sh
cd ${CUR_DIR}

























CORE_RANGE="32-63,96-127"
THREAD_NUM=$(numactl --hardware | awk '/node 0 cpus:/ {print (NF-3)/2}')
THREAD_NUM=$((2 * THREAD_NUM))
AB_HIGH_THREAD_NUM=22
AB_PRT=0.85
DAHA_SYMB_THRES=5000000
DAHA_NUMC_THRES=450000
# AB_ACC_THRES=1.50
GROUPBY_6_HASH_HYP_LOG=${DIR_PATH}/groupby_6_hash_hyp.log
rm $GROUPBY_6_HASH_HYP_LOG
GROUPBY_6_MKL_HYP_LOG=${DIR_PATH}/groupby_6_mkl_hyp.log
rm $GROUPBY_6_MKL_HYP_LOG
GROUPBY_6_PHJ_RDX_BC_HYP_LOG=${DIR_PATH}/groupby_6_phj_rdx_bc_hyp.log
rm $GROUPBY_6_PHJ_RDX_BC_HYP_LOG
GROUPBY_6_PHJ_RDX_BC_AGG_HYP_LOG=${DIR_PATH}/groupby_6_phj_rdx_bc_agg_hyp.log
rm $GROUPBY_6_PHJ_RDX_BC_AGG_HYP_LOG
GROUPBY_6_NPHJ_SC_HYP_LOG=${DIR_PATH}/groupby_6_nphj_sc_hyp.log
rm $GROUPBY_6_NPHJ_SC_HYP_LOG
GROUPBY_6_NPHJ_SC_AGGOTFOA_HYP_LOG=${DIR_PATH}/groupby_6_nphj_sc_aggotfoa_hyp.log
rm $GROUPBY_6_NPHJ_SC_AGGOTFOA_HYP_LOG
GROUPBY_6_DIM3_HYP_LOG=${DIR_PATH}/groupby_6_dim3_hyp.log
rm $GROUPBY_6_DIM3_HYP_LOG
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
	-DDAHA=true -DDAHA_REAR=true \
	-DDAHA_QUANTILE=0.99 \
	-DDAHA_SYMB_THRES=$DAHA_SYMB_THRES \
	-DDAHA_NUMC_THRES=$DAHA_NUMC_THRES \
	-DAB_ACC_THRES=$AB_ACC_THRES \
	-DDN_MIN=256 \
	-DGUD_ALPHA=0.5 -DGUD_MIN=256 -DHYB_THRES=0.95 \
	-DHASH_CONSTANT=2654435761 -DMKL_SORT=false -DMKL_ENHANCE=false \
	-DRUN_NUM=$RUN_NUM -DIN_TAMU=false -DIN_GROUPBY=true -DIN_SSJ=false \
	-DUSE_PAPI=false -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
	-DIN_DEBUG=false -DIN_EXAMINE=false
make -j $(( $(nproc) / 16 ))
cd ..
for dataset in ${groupby_dataset_list[@]}; do
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/hashspgemm $dataset >> $GROUPBY_6_HASH_HYP_LOG 2>&1
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/mkl_dcsrmultcsr $dataset >> $GROUPBY_6_MKL_HYP_LOG 2>&1
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/phj_rdx_bc $dataset >> $GROUPBY_6_PHJ_RDX_BC_HYP_LOG 2>&1
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/phj_rdx_bc_agg $dataset >> $GROUPBY_6_PHJ_RDX_BC_AGG_HYP_LOG 2>&1
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/nphj_sc $dataset >> $GROUPBY_6_NPHJ_SC_HYP_LOG 2>&1
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/nphj_sc_aggotfoa $dataset >> $GROUPBY_6_NPHJ_SC_AGGOTFOA_HYP_LOG 2>&1
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/dim3 $dataset >> $GROUPBY_6_DIM3_HYP_LOG 2>&1
done
bash clean.sh
cd ${CUR_DIR}



CORE_RANGE="32-63,96-127"
THREAD_NUM=$(numactl --hardware | awk '/node 0 cpus:/ {print (NF-3)/2}')
# THREAD_NUM=$((2 * THREAD_NUM))
AB_HIGH_THREAD_NUM=22
AB_PRT=0.85
DAHA_SYMB_THRES=5000000
DAHA_NUMC_THRES=450000
# AB_ACC_THRES=1.50
GROUPBY_6_HASH_LOG=${DIR_PATH}/groupby_6_hash.log
rm $GROUPBY_6_HASH_LOG
GROUPBY_6_MKL_LOG=${DIR_PATH}/groupby_6_mkl.log
rm $GROUPBY_6_MKL_LOG
GROUPBY_6_PHJ_RDX_BC_LOG=${DIR_PATH}/groupby_6_phj_rdx_bc.log
rm $GROUPBY_6_PHJ_RDX_BC_LOG
GROUPBY_6_PHJ_RDX_BC_AGG_LOG=${DIR_PATH}/groupby_6_phj_rdx_bc_agg.log
rm $GROUPBY_6_PHJ_RDX_BC_AGG_LOG
GROUPBY_6_NPHJ_SC_LOG=${DIR_PATH}/groupby_6_nphj_sc.log
rm $GROUPBY_6_NPHJ_SC_LOG
GROUPBY_6_NPHJ_SC_AGGOTFOA_LOG=${DIR_PATH}/groupby_6_nphj_sc_aggotfoa.log
rm $GROUPBY_6_NPHJ_SC_AGGOTFOA_LOG
GROUPBY_6_DIM3_LOG=${DIR_PATH}/groupby_6_dim3.log
rm $GROUPBY_6_DIM3_LOG
cd ../..
bash clean.sh
mkdir build
cd build
cmake .. -DTHREAD_NUM=$THREAD_NUM -DUSE_HYPERTHREADING=false  \
	-DNUMA_MASK_VAL=6 -DUSE_WEIGHTED_INTERLEAVING=true -DUSE_INTERLEAVING=false \
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
	-DHASH_CONSTANT=2654435761 -DMKL_SORT=false -DMKL_ENHANCE=false \
	-DRUN_NUM=$RUN_NUM -DIN_TAMU=false -DIN_GROUPBY=true -DIN_SSJ=false \
	-DUSE_PAPI=false -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
	-DIN_DEBUG=false -DIN_EXAMINE=false
make -j $(( $(nproc) / 16 ))
cd ..
for dataset in ${groupby_dataset_list[@]}; do
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/hashspgemm $dataset >> $GROUPBY_6_HASH_LOG 2>&1
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/mkl_dcsrmultcsr $dataset >> $GROUPBY_6_MKL_LOG 2>&1
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/phj_rdx_bc $dataset >> $GROUPBY_6_PHJ_RDX_BC_LOG 2>&1
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/phj_rdx_bc_agg $dataset >> $GROUPBY_6_PHJ_RDX_BC_AGG_LOG 2>&1
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/nphj_sc $dataset >> $GROUPBY_6_NPHJ_SC_LOG 2>&1
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/nphj_sc_aggotfoa $dataset >> $GROUPBY_6_NPHJ_SC_AGGOTFOA_LOG 2>&1
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/dim3 $dataset >> $GROUPBY_6_DIM3_LOG 2>&1
done
bash clean.sh
cd ${CUR_DIR}



















end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date
