date
start_time=$(date +%s)




CUR_DIR=$(pwd)
mkdir ../../logs/20251014-logs
cd ../../logs/20251014-logs


for mem in 1 6; do
	for algo in hash hsersc heap; do
		ln -s ../20251006-logs/er_${mem}_${algo}.log
		ln -s ../20251006-logs/g500_${mem}_${algo}.log
		ln -s ../20251006-logs/ssca_${mem}_${algo}.log
		ln -s ../20251006-logs/tamu_${mem}_${algo}.log
	done
done

for mem in 1 6; do
	for algo in mkl mkls; do
		ln -s ../20251007-logs/er_${mem}_${algo}.log
		ln -s ../20251007-logs/g500_${mem}_${algo}.log
		ln -s ../20251007-logs/ssca_${mem}_${algo}.log
		ln -s ../20251007-logs/tamu_${mem}_${algo}.log
	done
done

for mem in 1 6; do
	ln -s ../20251008-logs/er_${mem}_outer.log	er_${mem}_pb_1024_32.log
	ln -s ../20251008-logs/g500_${mem}_outer.log	g500_${mem}_pb_1024_32.log
	ln -s ../20251008-logs/ssca_${mem}_outer.log	ssca_${mem}_pb_1024_32.log
	ln -s ../20251008-logs/tamu_${mem}_outer.log	tamu_${mem}_pb_1024_32.log
done


for mem in 1 6; do
	for algo in ab_hybacc ab_hash ab_hsersc; do
		ln -s ../20251010-logs/er_${mem}_${algo}.log
		ln -s ../20251010-logs/g500_${mem}_${algo}.log
		ln -s ../20251010-logs/ssca_${mem}_${algo}.log
		ln -s ../20251010-logs/tamu_${mem}_${algo}.log
	done
done


cd $CUR_DIR



end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date