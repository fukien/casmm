# mxc


date
start_time=$(date +%s)

CUR_DIR=$(pwd)

RUN_NUM=5
NUMACTL="/home/huang/workspace/numactl/numactl"



DIR_PATH=${CUR_DIR}/../../logs/20251006-logs
mkdir -p $DIR_PATH








tamu_dataset_list=(
	"m133-b3"
	"shar_te2-b3"
	"ch8-8-b5"
	"shar_te2-b2"
	"ch7-8-b4"
	"ch7-8-b5"
	"ch8-8-b3"
	"n4c6-b8"
	"n4c6-b7"
	"ch7-9-b5"
	"ch7-9-b4"
	"ch8-8-b4"
	"debr"
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







TAMU_6_HASH_LOG=${DIR_PATH}/tamu_6_hash.log
rm $TAMU_6_HASH_LOG
THREAD_NUM=$(numactl --hardware | awk '/node 0 cpus:/ {print (NF-3)/2}')
CORE_RANGE="32-63,96-127"
cd ../..
bash clean.sh
mkdir build
cd build
cmake .. -DTHREAD_NUM=$THREAD_NUM -DUSE_HYPERTHREADING=true  \
	-DNUMA_MASK_VAL=6 -DUSE_WEIGHTED_INTERLEAVING=true \
	-DCSRC_WRITE_OPTIMIZED=false -DCSRC_PREPOPULATED=false -DUSE_HUGE=false \
	-DMEMSV=false -DMEMSVB=false -DMEM_MON=false \
	-DDN_MIN=256 \
	-DGUD_ALPHA=0.5 -DGUD_MIN=256 -DHYB_THRES=0.95 \
	-DHASH_CONSTANT=2654435761 -DMKL_SORT=false \
	-DRUN_NUM=$RUN_NUM -DIN_TAMU=true -DIN_GROUPBY=false -DIN_SSJ=false \
	-DUSE_PAPI=false -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
	-DIN_DEBUG=false -DIN_EXAMINE=false
make -j $(( $(nproc) / 16 ))
cd ..
for dataset in ${tamu_dataset_list[@]}; do
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/hashspgemm $dataset >> $TAMU_6_HASH_LOG 2>&1
done
bash clean.sh
cd ${CUR_DIR}







TAMU_6_HASH_HYP_LOG=${DIR_PATH}/tamu_6_hash_hyp.log
rm $TAMU_6_HASH_HYP_LOG
THREAD_NUM=$(numactl --hardware | awk '/node 0 cpus:/ {print (NF-3)/2}')
THREAD_NUM=$((2 * THREAD_NUM))
CORE_RANGE="32-63,96-127"
cd ../..
bash clean.sh
mkdir build
cd build
cmake .. -DTHREAD_NUM=$THREAD_NUM -DUSE_HYPERTHREADING=true  \
	-DNUMA_MASK_VAL=6 -DUSE_WEIGHTED_INTERLEAVING=true \
	-DCSRC_WRITE_OPTIMIZED=false -DCSRC_PREPOPULATED=false -DUSE_HUGE=false \
	-DMEMSV=false -DMEMSVB=false -DMEM_MON=false \
	-DDN_MIN=256 \
	-DGUD_ALPHA=0.5 -DGUD_MIN=256 -DHYB_THRES=0.95 \
	-DHASH_CONSTANT=2654435761 -DMKL_SORT=false \
	-DRUN_NUM=$RUN_NUM -DIN_TAMU=true -DIN_GROUPBY=false -DIN_SSJ=false \
	-DUSE_PAPI=false -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
	-DIN_DEBUG=false -DIN_EXAMINE=false
make -j $(( $(nproc) / 16 ))
cd ..
for dataset in ${tamu_dataset_list[@]}; do
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/hashspgemm $dataset >> $TAMU_6_HASH_HYP_LOG 2>&1
done
bash clean.sh
cd ${CUR_DIR}







TAMU_1_HASH_LOG=${DIR_PATH}/tamu_1_hash.log
rm $TAMU_1_HASH_LOG
THREAD_NUM=$(numactl --hardware | awk '/node 0 cpus:/ {print (NF-3)/2}')
CORE_RANGE="0-31,64-95"
cd ../..
bash clean.sh
mkdir build
cd build
cmake .. -DTHREAD_NUM=$THREAD_NUM -DUSE_HYPERTHREADING=true  \
	-DNUMA_MASK_VAL=1 -DUSE_WEIGHTED_INTERLEAVING=false \
	-DCSRC_WRITE_OPTIMIZED=false -DCSRC_PREPOPULATED=false -DUSE_HUGE=false \
	-DMEMSV=false -DMEMSVB=false -DMEM_MON=false \
	-DDN_MIN=256 \
	-DGUD_ALPHA=0.5 -DGUD_MIN=256 -DHYB_THRES=0.95 \
	-DHASH_CONSTANT=2654435761 -DMKL_SORT=false \
	-DRUN_NUM=$RUN_NUM -DIN_TAMU=true -DIN_GROUPBY=false -DIN_SSJ=false \
	-DUSE_PAPI=false -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
	-DIN_DEBUG=false -DIN_EXAMINE=false \
	-DCFG_PATH=config/mc/mxc0.cfg
make -j $(( $(nproc) / 16 ))
cd ..
for dataset in ${tamu_dataset_list[@]}; do
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/hashspgemm $dataset >> $TAMU_1_HASH_LOG 2>&1
done
bash clean.sh
cd ${CUR_DIR}








TAMU_1_HASH_HYP_LOG=${DIR_PATH}/tamu_1_hash_hyp.log
rm $TAMU_1_HASH_HYP_LOG
THREAD_NUM=$(numactl --hardware | awk '/node 0 cpus:/ {print (NF-3)/2}')
THREAD_NUM=$((2 * THREAD_NUM))
CORE_RANGE="0-31,64-95"
cd ../..
bash clean.sh
mkdir build
cd build
cmake .. -DTHREAD_NUM=$THREAD_NUM -DUSE_HYPERTHREADING=true  \
	-DNUMA_MASK_VAL=1 -DUSE_WEIGHTED_INTERLEAVING=false \
	-DCSRC_WRITE_OPTIMIZED=false -DCSRC_PREPOPULATED=false -DUSE_HUGE=false \
	-DMEMSV=false -DMEMSVB=false -DMEM_MON=false \
	-DDN_MIN=256 \
	-DGUD_ALPHA=0.5 -DGUD_MIN=256 -DHYB_THRES=0.95 \
	-DHASH_CONSTANT=2654435761 -DMKL_SORT=false \
	-DRUN_NUM=$RUN_NUM -DIN_TAMU=true -DIN_GROUPBY=false -DIN_SSJ=false \
	-DUSE_PAPI=false -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
	-DIN_DEBUG=false -DIN_EXAMINE=false \
	-DCFG_PATH=config/mc/mxc0.cfg
make -j $(( $(nproc) / 16 ))
cd ..
for dataset in ${tamu_dataset_list[@]}; do
	$NUMACTL --physcpubind=$CORE_RANGE ./bin/hashspgemm $dataset >> $TAMU_1_HASH_HYP_LOG 2>&1
done
bash clean.sh
cd ${CUR_DIR}















end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date