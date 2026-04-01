# mxc

date
start_time=$(date +%s)

bash gen_er.sh > gen_er.log
bash gen_g500.sh > gen_g500.log
bash gen_ssca.sh > gen_ssca.log
bash download_tamu.sh > download_tamu.log
bash parallel_tamu_refactor.sh > parallel_tamu_refactor.log

end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date