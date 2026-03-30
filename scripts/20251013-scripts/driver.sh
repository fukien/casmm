# mxc

date
start_time=$(date +%s)

bash rmat_spmm.sh > rmat_spmm.log
bash rmat_spmv.sh > rmat_spmv.log

end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date