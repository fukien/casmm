# mxc

date
start_time=$(date +%s)

bash rmat_mkl.sh > rmat_mkl.log
bash tamu_mkl.sh > tamu_mkl.log

end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date