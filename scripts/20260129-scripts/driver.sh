# mxc

date
start_time=$(date +%s)

CUR_DIR=$(pwd)


cd ../20260122-scripts
bash driver.sh > driver.log
cd ../20260123-scripts
bash driver.sh > driver.log
cd ../20260124-scripts
bash driver.sh > driver.log
cd ..20260125-scripts/
bash driver.sh > driver.log


bash softlink.sh > softlink.log
cd $CUR_DIR


end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date