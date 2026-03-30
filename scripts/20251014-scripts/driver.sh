# mxc

date
start_time=$(date +%s)

CUR_DIR=$(pwd)


cd ../20251006-scripts
bash driver.sh > driver.log
cd ../20251007-scripts
bash driver.sh > driver.log
cd ../20251008-scripts
bash driver.sh > driver.log
cd ..20251010-scripts/
bash driver.sh > driver.log


bash softlink.sh > softlink.log
cd $CUR_DIR


end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date