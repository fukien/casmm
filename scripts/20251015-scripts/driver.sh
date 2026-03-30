# mxc

date
start_time=$(date +%s)

CUR_DIR=$(pwd)




bash rmat_gud.sh > rmat_gud.log
bash rmat_ompdnr.sh > rmat_ompdnr.log
bash rmat_ompgdr.sh > rmat_ompgdr.log


bash softlink.sh > softlink.log


end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date