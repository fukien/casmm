# mxc

date
start_time=$(date +%s)

bash build_outer.sh > build_outer.log
bash rmat_outer.sh > rmat_outer.log
bash tamu_outer.sh > tamu_outer.log
cd ../../
bash clean.sh

end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date