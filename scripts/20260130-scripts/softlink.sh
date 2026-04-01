date
start_time=$(date +%s)




CUR_DIR=$(pwd)
mkdir ../../logs/20260130-logs
cd ../../logs/20260130-logs


for mem in 1 6; do
	for algo in ab_hash_daha; do
		ln -s ../20260125-logs/er_${mem}_${algo}.log
		ln -s ../20260125-logs/g500_${mem}_${algo}.log
		ln -s ../20260125-logs/ssca_${mem}_${algo}.log
	done
done

cd $CUR_DIR

end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date