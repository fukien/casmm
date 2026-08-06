
date
start_time=$(date +%s)

source ./common.inc.sh

declare -A ab_list=(
	["ab_hashspgemm_jp"]="ab_hash_jp"
	["ab_hashspgemm_jp_sat"]="ab_hash_jp_sat"
)
ab_bins=("ab_hashspgemm_jp" "ab_hashspgemm_jp_sat")

AB_THREADS=${AB_THREADS:-"64"}

banner
echo "AB thread counts: $AB_THREADS   (20250917 ran 64 only)"
echo


for mask in $MASKS; do
set_mask $mask
for nthread in $AB_THREADS; do
set_threads $nthread

	for bin in ${ab_bins[@]}; do
		rm -f ${DIR_PATH}/groupby_${mask}_${ab_list[$bin]}${SUFFIX}.log
	done

	tag="m${mask}_${nthread}t"

	build $DEFAULT_WI false ${ab_bins[@]}
	for dataset in ${groupby_dataset_list[@]}; do
		if [ "$mask" = "1" ] && [ "$dataset" = "ml20m" ]; then continue; fi
		for bin in ${ab_bins[@]}; do
			run_one $bin $dataset \
				${DIR_PATH}/groupby_${mask}_${ab_list[$bin]}${SUFFIX}.log $tag
		done
	done
	teardown

	if [ "$mask" = "1" ] && want_ml20m; then
		build true false ${ab_bins[@]}
		for bin in ${ab_bins[@]}; do
			run_one $bin ml20m \
				${DIR_PATH}/groupby_${mask}_${ab_list[$bin]}${SUFFIX}.log "${tag}_wi"
		done
		teardown
	fi

done
done


report_timeouts

echo
ls -la ${DIR_PATH}/*ab_hash_jp*.log 2> /dev/null

end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date
