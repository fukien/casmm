# mxc


date
start_time=$(date +%s)


RUN_NUM=4
NUMACTL="/home/huang/workspace/numactl/numactl"

CUR_DIR=$(pwd)


DIR_PATH=${CUR_DIR}/../../logs/20251016-logs
mkdir -p $DIR_PATH



tamu_dataset_list=(
	"hugetric-00020"
	"hugetric-00010"
	"hugetric-00000"
	"adaptive"
	"af_shell10"
	"af_2_k101"
	"af_0_k101"
	"af_1_k101"
	"af_5_k101"
	"af_3_k101"
	"af_4_k101"
	"af_shell7"
	"af_shell5"
	"af_shell9"
	"af_shell4"
	"af_shell8"
	"af_shell3"
	"af_shell1"
	"af_shell6"
	"af_shell2"
	"BenElechi1"
	"channel-500x100x100-b050"
	"Hardesty3"
	"Rucci1"
	"nlpkkt160"
	"Transport"
	"nlpkkt120"
	"CurlCurl_4"
	"Emilia_923"
	"nlpkkt80"
	"relat8"
	"GL7d18"
	"Geo_1438"
	"Bump_2911"
	"fcondp2"
	"Hardesty2"
	"Fault_639"
	"GL7d17"
	"333SP"
	"CO"
	"StocF-1465"
	"GL7d16"
	"NLR"
	"M6"
	"AS365"
	"packing-500x100x100-b050"
	"troll"
	"halfb"
	"fullb"
	"GL7d15"
	"auto"
	"bmwcra_1"
	"Serena"
	"delaunay_n23"
	"delaunay_n22"
	"msdoor"
	"ldoor"
	"nv2"
	"rgg_n_2_23_s0"
	"x104"
	"rgg_n_2_22_s0"
	"amazon0601"
	"rgg_n_2_21_s0"
	"ohne2"
	"cage15"
	"cage14"
	"amazon0505"
	"cage13"
	"test1"
	"radiation"
	"amazon0312"
	"Freescale1"
	"dielFilterV2real"
	"PR02R"
	"Hook_1498"
	"PFlow_742"
	"fem_hifreq_circuit"
	"dgreen"
	"dielFilterV2clx"
	"gsm_106857"
	"3Dspectralwave"
	"pkustk14"
	"Si87H76"
	"ss"
	"amazon-2008"
	"webbase-1M"
	"pre2"
	"vas_stokes_2M"
	"Ga10As10H30"
	"rel9"
	"vas_stokes_1M"
	"kkt_power"
	"c-big"
	"Ge87H76"
	"Ga19As19H42"
	"Ge99H100"
	"email-EuAll"
	"web-Google"
	"Linux_call_graph"
	"patents"
	"cnr-2000"
	"cit-Patents"
	"NotreDame_www"
	"web-NotreDame"
	"soc-sign-epinions"
	"wiki-talk-temporal"
)



THREAD_NUM=$(numactl --hardware | awk '/node 0 cpus:/ {print (NF-3)/2}')
THREAD_NUM=$((2 * THREAD_NUM))
CORE_RANGE="32-63,96-127"



AB_HIGH_THREAD_NUM=16
# AB_PRT=0.90
AB_PRT=0.85
# mask-6
DAHA_SYMB_THRES=6000000
DAHA_NUMC_THRES=800000




# mask-2
LLC_ER_2_AB_HASH_LOG=${DIR_PATH}/llc_er_2_ab_hash.log
rm $LLC_ER_2_AB_HASH_LOG
LLC_G500_2_AB_HASH_LOG=${DIR_PATH}/llc_g500_2_ab_hash.log
rm $LLC_G500_2_AB_HASH_LOG
LLC_SSCA_2_AB_HASH_LOG=${DIR_PATH}/llc_ssca_2_ab_hash.log
rm $LLC_SSCA_2_AB_HASH_LOG
cd ../..
bash clean.sh
mkdir build
cd build
cmake .. -DTHREAD_NUM=$THREAD_NUM -DUSE_HYPERTHREADING=true  \
	-DNUMA_MASK_VAL=2 -DUSE_WEIGHTED_INTERLEAVING=false -DUSE_INTERLEAVING=false \
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
	-DDN_MIN=256 \
	-DGUD_ALPHA=0.5 -DGUD_MIN=256 -DHYB_THRES=0.95 \
	-DHASH_CONSTANT=2654435761 -DMKL_SORT=false -DMKL_ENHANCE=false \
	-DRUN_NUM=$RUN_NUM -DIN_TAMU=false -DIN_GROUPBY=false -DIN_SSJ=false \
	-DUSE_PAPI=true -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
	-DIN_DEBUG=false -DIN_EXAMINE=false
make -j $(( $(nproc) / 16 ))
cd ..
for scale in 18 19 20 21; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="er_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $LLC_ER_2_AB_HASH_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="g500_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $LLC_G500_2_AB_HASH_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="ssca_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $LLC_SSCA_2_AB_HASH_LOG 2>&1
	done
done
bash clean.sh
cd ${CUR_DIR}


LLC_TAMU_2_AB_HASH_LOG=${DIR_PATH}/llc_tamu_2_ab_hash.log
rm $LLC_TAMU_2_AB_HASH_LOG
cd ../..
bash clean.sh
mkdir build
cd build
cmake .. -DTHREAD_NUM=$THREAD_NUM -DUSE_HYPERTHREADING=true  \
	-DNUMA_MASK_VAL=2 -DUSE_WEIGHTED_INTERLEAVING=false -DUSE_INTERLEAVING=false \
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
	-DDN_MIN=256 \
	-DGUD_ALPHA=0.5 -DGUD_MIN=256 -DHYB_THRES=0.95 \
	-DHASH_CONSTANT=2654435761 -DMKL_SORT=false -DMKL_ENHANCE=false \
	-DRUN_NUM=$RUN_NUM -DIN_TAMU=true -DIN_GROUPBY=false -DIN_SSJ=false \
	-DUSE_PAPI=true -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
	-DIN_DEBUG=false -DIN_EXAMINE=false
make -j $(( $(nproc) / 16 ))
cd ..
for dataset in ${tamu_dataset_list[@]}; do
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $LLC_TAMU_2_AB_HASH_LOG 2>&1
done
bash clean.sh
cd ${CUR_DIR}





# mask-4
LLC_ER_4_AB_HASH_LOG=${DIR_PATH}/llc_er_4_ab_hash.log
rm $LLC_ER_4_AB_HASH_LOG
LLC_G500_4_AB_HASH_LOG=${DIR_PATH}/llc_g500_4_ab_hash.log
rm $LLC_G500_4_AB_HASH_LOG
LLC_SSCA_4_AB_HASH_LOG=${DIR_PATH}/llc_ssca_4_ab_hash.log
rm $LLC_SSCA_4_AB_HASH_LOG
cd ../..
bash clean.sh
mkdir build
cd build
cmake .. -DTHREAD_NUM=$THREAD_NUM -DUSE_HYPERTHREADING=true  \
	-DNUMA_MASK_VAL=4 -DUSE_WEIGHTED_INTERLEAVING=false -DUSE_INTERLEAVING=false \
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
	-DDN_MIN=256 \
	-DGUD_ALPHA=0.5 -DGUD_MIN=256 -DHYB_THRES=0.95 \
	-DHASH_CONSTANT=2654435761 -DMKL_SORT=false -DMKL_ENHANCE=false \
	-DRUN_NUM=$RUN_NUM -DIN_TAMU=false -DIN_GROUPBY=false -DIN_SSJ=false \
	-DUSE_PAPI=true -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
	-DIN_DEBUG=false -DIN_EXAMINE=false
make -j $(( $(nproc) / 16 ))
cd ..
for scale in 18 19 20 21; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="er_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $LLC_ER_4_AB_HASH_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="g500_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $LLC_G500_4_AB_HASH_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="ssca_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $LLC_SSCA_4_AB_HASH_LOG 2>&1
	done
done
bash clean.sh
cd ${CUR_DIR}


LLC_TAMU_4_AB_HASH_LOG=${DIR_PATH}/llc_tamu_4_ab_hash.log
rm $LLC_TAMU_4_AB_HASH_LOG
cd ../..
bash clean.sh
mkdir build
cd build
cmake .. -DTHREAD_NUM=$THREAD_NUM -DUSE_HYPERTHREADING=true  \
	-DNUMA_MASK_VAL=4 -DUSE_WEIGHTED_INTERLEAVING=false -DUSE_INTERLEAVING=false \
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
	-DDN_MIN=256 \
	-DGUD_ALPHA=0.5 -DGUD_MIN=256 -DHYB_THRES=0.95 \
	-DHASH_CONSTANT=2654435761 -DMKL_SORT=false -DMKL_ENHANCE=false \
	-DRUN_NUM=$RUN_NUM -DIN_TAMU=true -DIN_GROUPBY=false -DIN_SSJ=false \
	-DUSE_PAPI=true -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
	-DIN_DEBUG=false -DIN_EXAMINE=false
make -j $(( $(nproc) / 16 ))
cd ..
for dataset in ${tamu_dataset_list[@]}; do
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $LLC_TAMU_4_AB_HASH_LOG 2>&1
done
bash clean.sh
cd ${CUR_DIR}










# mask-6
LLC_ER_6_AB_HASH_LOG=${DIR_PATH}/llc_er_6_ab_hash.log
rm $LLC_ER_6_AB_HASH_LOG
LLC_G500_6_AB_HASH_LOG=${DIR_PATH}/llc_g500_6_ab_hash.log
rm $LLC_G500_6_AB_HASH_LOG
LLC_SSCA_6_AB_HASH_LOG=${DIR_PATH}/llc_ssca_6_ab_hash.log
rm $LLC_SSCA_6_AB_HASH_LOG
cd ../..
bash clean.sh
mkdir build
cd build
cmake .. -DTHREAD_NUM=$THREAD_NUM -DUSE_HYPERTHREADING=true  \
	-DNUMA_MASK_VAL=6 -DUSE_WEIGHTED_INTERLEAVING=true -DUSE_INTERLEAVING=false \
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
	-DDN_MIN=256 \
	-DGUD_ALPHA=0.5 -DGUD_MIN=256 -DHYB_THRES=0.95 \
	-DHASH_CONSTANT=2654435761 -DMKL_SORT=false -DMKL_ENHANCE=false \
	-DRUN_NUM=$RUN_NUM -DIN_TAMU=false -DIN_GROUPBY=false -DIN_SSJ=false \
	-DUSE_PAPI=true -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
	-DIN_DEBUG=false -DIN_EXAMINE=false
make -j $(( $(nproc) / 16 ))
cd ..
for scale in 18 19 20 21; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="er_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $LLC_ER_6_AB_HASH_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="g500_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $LLC_G500_6_AB_HASH_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="ssca_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $LLC_SSCA_6_AB_HASH_LOG 2>&1
	done
done
bash clean.sh
cd ${CUR_DIR}


LLC_TAMU_6_AB_HASH_LOG=${DIR_PATH}/llc_tamu_6_ab_hash.log
rm $LLC_TAMU_6_AB_HASH_LOG
cd ../..
bash clean.sh
mkdir build
cd build
cmake .. -DTHREAD_NUM=$THREAD_NUM -DUSE_HYPERTHREADING=true  \
	-DNUMA_MASK_VAL=6 -DUSE_WEIGHTED_INTERLEAVING=true -DUSE_INTERLEAVING=false \
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
	-DDN_MIN=256 \
	-DGUD_ALPHA=0.5 -DGUD_MIN=256 -DHYB_THRES=0.95 \
	-DHASH_CONSTANT=2654435761 -DMKL_SORT=false -DMKL_ENHANCE=false \
	-DRUN_NUM=$RUN_NUM -DIN_TAMU=true -DIN_GROUPBY=false -DIN_SSJ=false \
	-DUSE_PAPI=true -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
	-DIN_DEBUG=false -DIN_EXAMINE=false
make -j $(( $(nproc) / 16 ))
cd ..
for dataset in ${tamu_dataset_list[@]}; do
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $LLC_TAMU_6_AB_HASH_LOG 2>&1
done
bash clean.sh
cd ${CUR_DIR}







# mask-6ms
LLC_ER_6MS_AB_HASH_LOG=${DIR_PATH}/llc_er_6ms_ab_hash.log
rm $LLC_ER_6MS_AB_HASH_LOG
LLC_G500_6MS_AB_HASH_LOG=${DIR_PATH}/llc_g500_6ms_ab_hash.log
rm $LLC_G500_6MS_AB_HASH_LOG
LLC_SSCA_6MS_AB_HASH_LOG=${DIR_PATH}/llc_ssca_6ms_ab_hash.log
rm $LLC_SSCA_6MS_AB_HASH_LOG
cd ../..
bash clean.sh
mkdir build
cd build
cmake .. -DTHREAD_NUM=$THREAD_NUM -DUSE_HYPERTHREADING=true  \
	-DNUMA_MASK_VAL=6 -DUSE_WEIGHTED_INTERLEAVING=true -DUSE_INTERLEAVING=false \
	-DCSRC_WRITE_OPTIMIZED=false -DCSRC_PREPOPULATED=false -DUSE_HUGE=false \
	-DMEMSV=true -DMEMSVB=false -DMEM_MON=false \
	-DAB_HIGH_THREAD_NUM=$AB_HIGH_THREAD_NUM -DAB_SIZE=524288 -DAB_MIN_NUM=32768 \
	-DAB_LPT=false -DAB_SIMP=false -DAB_PRT=$AB_PRT -DAB_MAXFLP=false \
	-DAB_DNR=true \
	-DAB_HYOPT=false -DAB_SYMB_THRES=66.0 -DAB_NUMC_THRES=1.50 \
	-DDAHA=true -DDAHA_REAR=true \
	-DDAHA_QUANTILE=0.99 \
	-DDAHA_SYMB_THRES=$DAHA_SYMB_THRES \
	-DDAHA_NUMC_THRES=$DAHA_NUMC_THRES \
	-DDN_MIN=256 \
	-DGUD_ALPHA=0.5 -DGUD_MIN=256 -DHYB_THRES=0.95 \
	-DHASH_CONSTANT=2654435761 -DMKL_SORT=false -DMKL_ENHANCE=false \
	-DRUN_NUM=$RUN_NUM -DIN_TAMU=false -DIN_GROUPBY=false -DIN_SSJ=false \
	-DUSE_PAPI=true -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
	-DIN_DEBUG=false -DIN_EXAMINE=false
make -j $(( $(nproc) / 16 ))
cd ..
for scale in 18 19 20 21; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="er_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $LLC_ER_6MS_AB_HASH_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="g500_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $LLC_G500_6MS_AB_HASH_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="ssca_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $LLC_SSCA_6MS_AB_HASH_LOG 2>&1
	done
done
bash clean.sh
cd ${CUR_DIR}


LLC_TAMU_6MS_AB_HASH_LOG=${DIR_PATH}/llc_tamu_6ms_ab_hash.log
rm $LLC_TAMU_6MS_AB_HASH_LOG
cd ../..
bash clean.sh
mkdir build
cd build
cmake .. -DTHREAD_NUM=$THREAD_NUM -DUSE_HYPERTHREADING=true  \
	-DNUMA_MASK_VAL=6 -DUSE_WEIGHTED_INTERLEAVING=true -DUSE_INTERLEAVING=false \
	-DCSRC_WRITE_OPTIMIZED=false -DCSRC_PREPOPULATED=false -DUSE_HUGE=false \
	-DMEMSV=true -DMEMSVB=false -DMEM_MON=false \
	-DAB_HIGH_THREAD_NUM=$AB_HIGH_THREAD_NUM -DAB_SIZE=524288 -DAB_MIN_NUM=32768 \
	-DAB_LPT=false -DAB_SIMP=false -DAB_PRT=$AB_PRT -DAB_MAXFLP=false \
	-DAB_DNR=true \
	-DAB_HYOPT=false -DAB_SYMB_THRES=66.0 -DAB_NUMC_THRES=1.50 \
	-DDAHA=true -DDAHA_REAR=true \
	-DDAHA_QUANTILE=0.99 \
	-DDAHA_SYMB_THRES=$DAHA_SYMB_THRES \
	-DDAHA_NUMC_THRES=$DAHA_NUMC_THRES \
	-DDN_MIN=256 \
	-DGUD_ALPHA=0.5 -DGUD_MIN=256 -DHYB_THRES=0.95 \
	-DHASH_CONSTANT=2654435761 -DMKL_SORT=false -DMKL_ENHANCE=false \
	-DRUN_NUM=$RUN_NUM -DIN_TAMU=true -DIN_GROUPBY=false -DIN_SSJ=false \
	-DUSE_PAPI=true -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
	-DIN_DEBUG=false -DIN_EXAMINE=false
make -j $(( $(nproc) / 16 ))
cd ..
for dataset in ${tamu_dataset_list[@]}; do
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $LLC_TAMU_6MS_AB_HASH_LOG 2>&1
done
bash clean.sh
cd ${CUR_DIR}








# mask-2ms
LLC_ER_2MS_AB_HASH_LOG=${DIR_PATH}/llc_er_2ms_ab_hash.log
rm $LLC_ER_2MS_AB_HASH_LOG
LLC_G500_2MS_AB_HASH_LOG=${DIR_PATH}/llc_g500_2ms_ab_hash.log
rm $LLC_G500_2MS_AB_HASH_LOG
LLC_SSCA_2MS_AB_HASH_LOG=${DIR_PATH}/llc_ssca_2ms_ab_hash.log
rm $LLC_SSCA_2MS_AB_HASH_LOG
cd ../..
bash clean.sh
mkdir build
cd build
cmake .. -DTHREAD_NUM=$THREAD_NUM -DUSE_HYPERTHREADING=true  \
	-DNUMA_MASK_VAL=2 -DUSE_WEIGHTED_INTERLEAVING=false -DUSE_INTERLEAVING=false \
	-DCSRC_WRITE_OPTIMIZED=false -DCSRC_PREPOPULATED=false -DUSE_HUGE=false \
	-DMEMSV=true -DMEMSVB=false -DMEM_MON=false \
	-DAB_HIGH_THREAD_NUM=$AB_HIGH_THREAD_NUM -DAB_SIZE=524288 -DAB_MIN_NUM=32768 \
	-DAB_LPT=false -DAB_SIMP=false -DAB_PRT=$AB_PRT -DAB_MAXFLP=false \
	-DAB_DNR=true \
	-DAB_HYOPT=false -DAB_SYMB_THRES=66.0 -DAB_NUMC_THRES=1.50 \
	-DDAHA=true -DDAHA_REAR=true \
	-DDAHA_QUANTILE=0.99 \
	-DDAHA_SYMB_THRES=$DAHA_SYMB_THRES \
	-DDAHA_NUMC_THRES=$DAHA_NUMC_THRES \
	-DDN_MIN=256 \
	-DGUD_ALPHA=0.5 -DGUD_MIN=256 -DHYB_THRES=0.95 \
	-DHASH_CONSTANT=2654435761 -DMKL_SORT=false -DMKL_ENHANCE=false \
	-DRUN_NUM=$RUN_NUM -DIN_TAMU=false -DIN_GROUPBY=false -DIN_SSJ=false \
	-DUSE_PAPI=true -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
	-DIN_DEBUG=false -DIN_EXAMINE=false
make -j $(( $(nproc) / 16 ))
cd ..
for scale in 18 19 20 21; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="er_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $LLC_ER_2MS_AB_HASH_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="g500_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $LLC_G500_2MS_AB_HASH_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="ssca_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $LLC_SSCA_2MS_AB_HASH_LOG 2>&1
	done
done
bash clean.sh
cd ${CUR_DIR}


LLC_TAMU_2MS_AB_HASH_LOG=${DIR_PATH}/llc_tamu_2ms_ab_hash.log
rm $LLC_TAMU_2MS_AB_HASH_LOG
cd ../..
bash clean.sh
mkdir build
cd build
cmake .. -DTHREAD_NUM=$THREAD_NUM -DUSE_HYPERTHREADING=true  \
	-DNUMA_MASK_VAL=2 -DUSE_WEIGHTED_INTERLEAVING=false -DUSE_INTERLEAVING=false \
	-DCSRC_WRITE_OPTIMIZED=false -DCSRC_PREPOPULATED=false -DUSE_HUGE=false \
	-DMEMSV=true -DMEMSVB=false -DMEM_MON=false \
	-DAB_HIGH_THREAD_NUM=$AB_HIGH_THREAD_NUM -DAB_SIZE=524288 -DAB_MIN_NUM=32768 \
	-DAB_LPT=false -DAB_SIMP=false -DAB_PRT=$AB_PRT -DAB_MAXFLP=false \
	-DAB_DNR=true \
	-DAB_HYOPT=false -DAB_SYMB_THRES=66.0 -DAB_NUMC_THRES=1.50 \
	-DDAHA=true -DDAHA_REAR=true \
	-DDAHA_QUANTILE=0.99 \
	-DDAHA_SYMB_THRES=$DAHA_SYMB_THRES \
	-DDAHA_NUMC_THRES=$DAHA_NUMC_THRES \
	-DDN_MIN=256 \
	-DGUD_ALPHA=0.5 -DGUD_MIN=256 -DHYB_THRES=0.95 \
	-DHASH_CONSTANT=2654435761 -DMKL_SORT=false -DMKL_ENHANCE=false \
	-DRUN_NUM=$RUN_NUM -DIN_TAMU=true -DIN_GROUPBY=false -DIN_SSJ=false \
	-DUSE_PAPI=true -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
	-DIN_DEBUG=false -DIN_EXAMINE=false
make -j $(( $(nproc) / 16 ))
cd ..
for dataset in ${tamu_dataset_list[@]}; do
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $LLC_TAMU_2MS_AB_HASH_LOG 2>&1
done
bash clean.sh
cd ${CUR_DIR}








# mask-2msb
LLC_ER_2MSB_AB_HASH_LOG=${DIR_PATH}/llc_er_2msb_ab_hash.log
rm $LLC_ER_2MSB_AB_HASH_LOG
LLC_G500_2MSB_AB_HASH_LOG=${DIR_PATH}/llc_g500_2msb_ab_hash.log
rm $LLC_G500_2MSB_AB_HASH_LOG
LLC_SSCA_2MSB_AB_HASH_LOG=${DIR_PATH}/llc_ssca_2msb_ab_hash.log
rm $LLC_SSCA_2MSB_AB_HASH_LOG
cd ../..
bash clean.sh
mkdir build
cd build
cmake .. -DTHREAD_NUM=$THREAD_NUM -DUSE_HYPERTHREADING=true  \
	-DNUMA_MASK_VAL=2 -DUSE_WEIGHTED_INTERLEAVING=false -DUSE_INTERLEAVING=false \
	-DCSRC_WRITE_OPTIMIZED=false -DCSRC_PREPOPULATED=false -DUSE_HUGE=false \
	-DMEMSV=true -DMEMSVB=true -DMEM_MON=false \
	-DAB_HIGH_THREAD_NUM=$AB_HIGH_THREAD_NUM -DAB_SIZE=524288 -DAB_MIN_NUM=32768 \
	-DAB_LPT=false -DAB_SIMP=false -DAB_PRT=$AB_PRT -DAB_MAXFLP=false \
	-DAB_DNR=true \
	-DAB_HYOPT=false -DAB_SYMB_THRES=66.0 -DAB_NUMC_THRES=1.50 \
	-DDAHA=true -DDAHA_REAR=true \
	-DDAHA_QUANTILE=0.99 \
	-DDAHA_SYMB_THRES=$DAHA_SYMB_THRES \
	-DDAHA_NUMC_THRES=$DAHA_NUMC_THRES \
	-DDN_MIN=256 \
	-DGUD_ALPHA=0.5 -DGUD_MIN=256 -DHYB_THRES=0.95 \
	-DHASH_CONSTANT=2654435761 -DMKL_SORT=false -DMKL_ENHANCE=false \
	-DRUN_NUM=$RUN_NUM -DIN_TAMU=false -DIN_GROUPBY=false -DIN_SSJ=false \
	-DUSE_PAPI=true -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
	-DIN_DEBUG=false -DIN_EXAMINE=false
make -j $(( $(nproc) / 16 ))
cd ..
for scale in 18 19 20 21; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="er_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $LLC_ER_2MSB_AB_HASH_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="g500_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $LLC_G500_2MSB_AB_HASH_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="ssca_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $LLC_SSCA_2MSB_AB_HASH_LOG 2>&1
	done
done
bash clean.sh
cd ${CUR_DIR}


LLC_TAMU_2MSB_AB_HASH_LOG=${DIR_PATH}/llc_tamu_2msb_ab_hash.log
rm $LLC_TAMU_2MSB_AB_HASH_LOG
cd ../..
bash clean.sh
mkdir build
cd build
cmake .. -DTHREAD_NUM=$THREAD_NUM -DUSE_HYPERTHREADING=true  \
	-DNUMA_MASK_VAL=2 -DUSE_WEIGHTED_INTERLEAVING=false -DUSE_INTERLEAVING=false \
	-DCSRC_WRITE_OPTIMIZED=false -DCSRC_PREPOPULATED=false -DUSE_HUGE=false \
	-DMEMSV=true -DMEMSVB=true -DMEM_MON=false \
	-DAB_HIGH_THREAD_NUM=$AB_HIGH_THREAD_NUM -DAB_SIZE=524288 -DAB_MIN_NUM=32768 \
	-DAB_LPT=false -DAB_SIMP=false -DAB_PRT=$AB_PRT -DAB_MAXFLP=false \
	-DAB_DNR=true \
	-DAB_HYOPT=false -DAB_SYMB_THRES=66.0 -DAB_NUMC_THRES=1.50 \
	-DDAHA=true -DDAHA_REAR=true \
	-DDAHA_QUANTILE=0.99 \
	-DDAHA_SYMB_THRES=$DAHA_SYMB_THRES \
	-DDAHA_NUMC_THRES=$DAHA_NUMC_THRES \
	-DDN_MIN=256 \
	-DGUD_ALPHA=0.5 -DGUD_MIN=256 -DHYB_THRES=0.95 \
	-DHASH_CONSTANT=2654435761 -DMKL_SORT=false -DMKL_ENHANCE=false \
	-DRUN_NUM=$RUN_NUM -DIN_TAMU=true -DIN_GROUPBY=false -DIN_SSJ=false \
	-DUSE_PAPI=true -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
	-DIN_DEBUG=false -DIN_EXAMINE=false
make -j $(( $(nproc) / 16 ))
cd ..
for dataset in ${tamu_dataset_list[@]}; do
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $LLC_TAMU_2MSB_AB_HASH_LOG 2>&1
done
bash clean.sh
cd ${CUR_DIR}








# mask-x
LLC_ER_X_AB_HASH_LOG=${DIR_PATH}/llc_er_x_ab_hash.log
rm $LLC_ER_X_AB_HASH_LOG
LLC_G500_X_AB_HASH_LOG=${DIR_PATH}/llc_g500_x_ab_hash.log
rm $LLC_G500_X_AB_HASH_LOG
LLC_SSCA_X_AB_HASH_LOG=${DIR_PATH}/llc_ssca_x_ab_hash.log
rm $LLC_SSCA_X_AB_HASH_LOG
cd ../..
bash clean.sh
mkdir build
cd build
cmake .. -DTHREAD_NUM=$THREAD_NUM -DUSE_HYPERTHREADING=true  \
	-DNUMA_MASK_VAL=1 -DUSE_WEIGHTED_INTERLEAVING=false -DUSE_INTERLEAVING=false \
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
	-DDN_MIN=256 \
	-DGUD_ALPHA=0.5 -DGUD_MIN=256 -DHYB_THRES=0.95 \
	-DHASH_CONSTANT=2654435761 -DMKL_SORT=false -DMKL_ENHANCE=false \
	-DRUN_NUM=$RUN_NUM -DIN_TAMU=false -DIN_GROUPBY=false -DIN_SSJ=false \
	-DUSE_PAPI=true -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
	-DIN_DEBUG=false -DIN_EXAMINE=false
make -j $(( $(nproc) / 16 ))
cd ..
for scale in 18 19 20 21; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="er_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $LLC_ER_X_AB_HASH_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="g500_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $LLC_G500_X_AB_HASH_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="ssca_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $LLC_SSCA_X_AB_HASH_LOG 2>&1
	done
done
bash clean.sh
cd ${CUR_DIR}


LLC_TAMU_X_AB_HASH_LOG=${DIR_PATH}/llc_tamu_x_ab_hash.log
rm $LLC_TAMU_X_AB_HASH_LOG
cd ../..
bash clean.sh
mkdir build
cd build
cmake .. -DTHREAD_NUM=$THREAD_NUM -DUSE_HYPERTHREADING=true  \
	-DNUMA_MASK_VAL=1 -DUSE_WEIGHTED_INTERLEAVING=false -DUSE_INTERLEAVING=false \
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
	-DDN_MIN=256 \
	-DGUD_ALPHA=0.5 -DGUD_MIN=256 -DHYB_THRES=0.95 \
	-DHASH_CONSTANT=2654435761 -DMKL_SORT=false -DMKL_ENHANCE=false \
	-DRUN_NUM=$RUN_NUM -DIN_TAMU=true -DIN_GROUPBY=false -DIN_SSJ=false \
	-DUSE_PAPI=true -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
	-DIN_DEBUG=false -DIN_EXAMINE=false
make -j $(( $(nproc) / 16 ))
cd ..
for dataset in ${tamu_dataset_list[@]}; do
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $LLC_TAMU_X_AB_HASH_LOG 2>&1
done
bash clean.sh
cd ${CUR_DIR}



































THREAD_NUM=$(numactl --hardware | awk '/node 0 cpus:/ {print (NF-3)/2}')
THREAD_NUM=$((2 * THREAD_NUM))
CORE_RANGE="0-31,64-95"



AB_HIGH_THREAD_NUM=16
# AB_PRT=0.90
AB_PRT=0.85
# mask-1
DAHA_SYMB_THRES=11000000
DAHA_NUMC_THRES=2500000


# mask-1
LLC_ER_1_AB_HASH_LOG=${DIR_PATH}/llc_er_1_ab_hash.log
rm $LLC_ER_1_AB_HASH_LOG
LLC_G500_1_AB_HASH_LOG=${DIR_PATH}/llc_g500_1_ab_hash.log
rm $LLC_G500_1_AB_HASH_LOG
LLC_SSCA_1_AB_HASH_LOG=${DIR_PATH}/llc_ssca_1_ab_hash.log
rm $LLC_SSCA_1_AB_HASH_LOG
cd ../..
bash clean.sh
mkdir build
cd build
cmake .. -DTHREAD_NUM=$THREAD_NUM -DUSE_HYPERTHREADING=true  \
	-DNUMA_MASK_VAL=1 -DUSE_WEIGHTED_INTERLEAVING=false -DUSE_INTERLEAVING=false \
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
	-DDN_MIN=256 \
	-DGUD_ALPHA=0.5 -DGUD_MIN=256 -DHYB_THRES=0.95 \
	-DHASH_CONSTANT=2654435761 -DMKL_SORT=false -DMKL_ENHANCE=false \
	-DRUN_NUM=$RUN_NUM -DIN_TAMU=false -DIN_GROUPBY=false -DIN_SSJ=false \
	-DUSE_PAPI=true -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
	-DIN_DEBUG=false -DIN_EXAMINE=false \
	-DCFG_PATH=config/mc/mxc0.cfg
make -j $(( $(nproc) / 16 ))
cd ..
for scale in 18 19 20 21; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="er_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $LLC_ER_1_AB_HASH_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="g500_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $LLC_G500_1_AB_HASH_LOG 2>&1
	done
done
for scale in 17 18 19 20; do
	for edge_factor in 1 2 4 8 16 32; do
		dataset="ssca_${scale}_${edge_factor}"
		$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $LLC_SSCA_1_AB_HASH_LOG 2>&1
	done
done
bash clean.sh
cd ${CUR_DIR}


LLC_TAMU_1_AB_HASH_LOG=${DIR_PATH}/llc_tamu_1_ab_hash.log
rm $LLC_TAMU_1_AB_HASH_LOG
cd ../..
bash clean.sh
mkdir build
cd build
cmake .. -DTHREAD_NUM=$THREAD_NUM -DUSE_HYPERTHREADING=true  \
	-DNUMA_MASK_VAL=1 -DUSE_WEIGHTED_INTERLEAVING=false -DUSE_INTERLEAVING=false \
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
	-DDN_MIN=256 \
	-DGUD_ALPHA=0.5 -DGUD_MIN=256 -DHYB_THRES=0.95 \
	-DHASH_CONSTANT=2654435761 -DMKL_SORT=false -DMKL_ENHANCE=false \
	-DRUN_NUM=$RUN_NUM -DIN_TAMU=true -DIN_GROUPBY=false -DIN_SSJ=false \
	-DUSE_PAPI=true -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
	-DIN_DEBUG=false -DIN_EXAMINE=false \
	-DCFG_PATH=config/mc/mxc0.cfg
make -j $(( $(nproc) / 16 ))
cd ..
for dataset in ${tamu_dataset_list[@]}; do
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/ab_hashspgemm $dataset >> $LLC_TAMU_1_AB_HASH_LOG 2>&1
done
bash clean.sh
cd ${CUR_DIR}












end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date
