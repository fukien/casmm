# mxc

date
start_time=$(date +%s)

bash rmat_ab_hybacc.sh > rmat_ab_hybacc.log
bash rmat_ab_hybacc_hyp.sh > rmat_ab_hybacc_hyp.log
bash softlink.sh > softlink.log

end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date