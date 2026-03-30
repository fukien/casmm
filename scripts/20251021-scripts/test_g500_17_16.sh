# mxc


date
start_time=$(date +%s)


CUR_DIR=$(pwd)


DIR_PATH=${CUR_DIR}/../../logs/20251021-logs
mkdir -p $DIR_PATH

CORE_RANGE="32-63"




G500_17_16_DEMO_HASH_2_LOG=${DIR_PATH}/g500_17_16_demo_hash_2.log
rm $G500_17_16_DEMO_HASH_2_LOG
cd ../..
bash clean.sh
mkdir build
cd build
cmake .. -DCSRC_WRITE_OPTIMIZED=false -DCSRC_PREPOPULATED=false -DUSE_HUGE=false -DTHREAD_NUM=32 \
        -DUSE_HYPERTHREADING=true -DNUMA_MASK_VAL=2 -DUSE_WEIGHTED_INTERLEAVING=true -DUSE_INTERLEAVING=false \
        -DCF_MARK_0=1.7 -DCF_MARK_1=2.2 -DRUN_NUM=5 -DIN_TAMU=false -DIN_GROUPBY=false -DIN_SSJ=false \
        -DMKL_SORT=false -DUSE_PAPI=false -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
        -DIN_DEBUG=true -DHASH_CONSTANT=3
make -j $(( $(nproc) / 16 ))
cd ..
numactl --physcpubind=$CORE_RANGE ./bin/hashspgemm g500_17_16 >> $G500_17_16_DEMO_HASH_2_LOG 2>&1
bash clean.sh
cd ${CUR_DIR}


G500_17_16_DEMO_HASH_6_LOG=${DIR_PATH}/g500_17_16_demo_hash_6.log
rm $G500_17_16_DEMO_HASH_6_LOG
cd ../..
bash clean.sh
mkdir build
cd build
cmake .. -DCSRC_WRITE_OPTIMIZED=false -DCSRC_PREPOPULATED=false -DUSE_HUGE=false -DTHREAD_NUM=32 \
        -DUSE_HYPERTHREADING=true -DNUMA_MASK_VAL=6 -DUSE_WEIGHTED_INTERLEAVING=true -DUSE_INTERLEAVING=false \
        -DCF_MARK_0=1.7 -DCF_MARK_1=2.2 -DRUN_NUM=5 -DIN_TAMU=false -DIN_GROUPBY=false -DIN_SSJ=false \
        -DMKL_SORT=false -DUSE_PAPI=false -DIN_INSP=false -DIN_STATS=false -DIN_VERIFY=false \
        -DIN_DEBUG=true -DHASH_CONSTANT=3
make -j $(( $(nproc) / 16 ))
cd ..
numactl --physcpubind=$CORE_RANGE ./bin/hashspgemm g500_17_16 >> $G500_17_16_DEMO_HASH_6_LOG 2>&1
bash clean.sh
cd ${CUR_DIR}





end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date