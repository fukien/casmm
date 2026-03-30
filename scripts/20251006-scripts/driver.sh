# mxc

date
start_time=$(date +%s)

bash rmat_hash.sh > rmat_hash.log
bash rmat_hsersc.sh > rmat_hsersc.log
bash rmat_heap.sh > rmat_heap.log

bash tamu_hash.sh > tamu_hash.log
bash tamu_hsersc.sh > tamu_hsersc.log
bash tamu_heap.sh > tamu_heap.log

end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date