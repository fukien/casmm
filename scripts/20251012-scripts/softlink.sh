date
start_time=$(date +%s)




CUR_DIR=$(pwd)
mkdir ../../logs/20251012-logs
cd ../../logs/20251012-logs



ln -s ../20251010-logs/er_1_ab_hybacc_daha.log
ln -s ../20251010-logs/g500_1_ab_hybacc_daha.log
ln -s ../20251010-logs/ssca_1_ab_hybacc_daha.log







cd ${CUR_DIR}




end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date