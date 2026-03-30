date
start_time=$(date +%s)




CUR_DIR=$(pwd)
mkdir ../../logs/20251018-logs
cd ../../logs/20251018-logs




for algo in hash hsersc; do
	ln -s ../20251008-logs/er_6_${algo}.log
	ln -s ../20251008-logs/g500_6_${algo}.log
	ln -s ../20251008-logs/ssca_6_${algo}.log
done


cd $CUR_DIR


end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date