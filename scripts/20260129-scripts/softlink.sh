date
start_time=$(date +%s)




CUR_DIR=$(pwd)
mkdir ../../logs/20260129-logs
cd ../../logs/20260129-logs


for mem in 1 6; do
	for algo in hash hsersc heap; do
		ln -s ../20260122-logs/er_${mem}_${algo}.log
		ln -s ../20260122-logs/g500_${mem}_${algo}.log
		ln -s ../20260122-logs/ssca_${mem}_${algo}.log
		ln -s ../20260122-logs/tamu_${mem}_${algo}.log
	done
done

for mem in 1 6; do
	for algo in mkl mkls; do
		ln -s ../20260123-logs/er_${mem}_${algo}.log
		ln -s ../20260123-logs/g500_${mem}_${algo}.log
		ln -s ../20260123-logs/ssca_${mem}_${algo}.log
		ln -s ../20260123-logs/tamu_${mem}_${algo}.log
	done
done

for mem in 1 6; do
	ln -s ../20260124-logs/er_${mem}_outer.log	er_${mem}_pb_1024_32.log
	ln -s ../20260124-logs/g500_${mem}_outer.log	g500_${mem}_pb_1024_32.log
	ln -s ../20260124-logs/ssca_${mem}_outer.log	ssca_${mem}_pb_1024_32.log
	ln -s ../20260124-logs/tamu_${mem}_outer.log	tamu_${mem}_pb_1024_32.log
done


for mem in 1 6; do
	for algo in ab_hybacc ab_hash ab_hsersc; do
		ln -s ../20260125-logs/er_${mem}_${algo}.log
		ln -s ../20260125-logs/g500_${mem}_${algo}.log
		ln -s ../20260125-logs/ssca_${mem}_${algo}.log
		ln -s ../20260125-logs/tamu_${mem}_${algo}.log
	done
done


cd $CUR_DIR



end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date