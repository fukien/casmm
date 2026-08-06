
date
start_time=$(date +%s)

source ./common.inc.sh

dim3_bins=("dim3_jp")
DIM3_STEM="dim3_jp"

banner


for mask in $MASKS; do
set_mask $mask
for nthread in $THREADS; do
set_threads $nthread

	rm -f ${DIR_PATH}/groupby_${mask}_${DIM3_STEM}${SUFFIX}.log

	tag="m${mask}_${nthread}t"

	build $DEFAULT_WI false ${dim3_bins[@]}
	for dataset in ${groupby_dataset_list[@]}; do
		if [ "$mask" = "1" ] && [ "$dataset" = "ml20m" ]; then continue; fi
		run_one dim3_jp $dataset \
			${DIR_PATH}/groupby_${mask}_${DIM3_STEM}${SUFFIX}.log $tag
	done
	teardown

	if [ "$mask" = "1" ] && want_ml20m; then
		build true false ${dim3_bins[@]}
		run_one dim3_jp ml20m \
			${DIR_PATH}/groupby_${mask}_${DIM3_STEM}${SUFFIX}.log "${tag}_wi"
		teardown
	fi

done
done


report_timeouts

echo
ls -la ${DIR_PATH}/*dim3* 2> /dev/null

end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date
