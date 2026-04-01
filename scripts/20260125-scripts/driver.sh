# mxc

date
start_time=$(date +%s)

bash rmat_ab_daha.sh > rmat_ab_daha.log
bash rmat_ab_hybacc_daha.sh > rmat_ab_hybacc_daha.log
bash tamu_ab_daha.sh > tamu_ab_daha.log
bash tamu_ab_hybacc_daha.sh > tamu_ab_hybacc_daha.log

end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date