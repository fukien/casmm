date
start_time=$(date +%s)


bash llc_rmat_ab_hybacc_daha.sh > llc_rmat_ab_hybacc_daha.log
bash llc_ab_hash.sh > llc_ab_hash.log
bash llc_ab_hsersc.sh > llc_ab_hsersc.log
bash llc_hash.sh > llc_hash.log
bash llc_hsersc.sh > llc_hsersc.log


end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date