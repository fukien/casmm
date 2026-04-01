# mxc

date
start_time=$(date +%s)

bash rmat_6_ab.sh > rmat_6_ab.log
bash rmat_6_ab_lpt.sh > rmat_6_ab_lpt.log
bash rmat_6_ab_simp.sh > rmat_6_ab_simp.log

end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date