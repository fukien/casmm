date
start_time=$(date +%s)




CUR_DIR=$(pwd)
mkdir ../../logs/20251016-logs
cd ../../logs/20251016-logs




for algo in ab_hybacc_daha ab_hash_daha ab_hsersc_daha; do
	ln -s ../20251010-logs/g500_6_${algo}.log
done
for algo in hash hsersc; do
	ln -s ../20251008-logs/g500_6_${algo}.log
done


cd $CUR_DIR


end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date