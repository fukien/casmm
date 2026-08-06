
date
start_time=$(date +%s)

source ./common.inc.sh

declare -A mkl_list=(
	["mkl_sparse_sp2m_jp"]="mkl_sp2m_jp"
)
mkl_bins=("mkl_sparse_sp2m_jp")

if [ -n "$WITH_JA" ]; then
	mkl_list["mkl_sparse_sp2m"]="mkl_sp2m"
	mkl_bins+=("mkl_sparse_sp2m")
fi

banner
echo "sort modes: false only (sp2m has no sort parameter -- see header)"
echo


for mask in $MASKS; do
set_mask $mask
for nthread in $THREADS; do
set_threads $nthread

	for bin in ${mkl_bins[@]}; do
		rm -f ${DIR_PATH}/groupby_${mask}_${mkl_list[$bin]}${SUFFIX}.log
	done

	tag="m${mask}_${nthread}t"

	build $DEFAULT_WI false ${mkl_bins[@]}
	for dataset in ${groupby_dataset_list[@]}; do
		if [ "$mask" = "1" ] && [ "$dataset" = "ml20m" ]; then continue; fi
		for bin in ${mkl_bins[@]}; do
			run_one $bin $dataset \
				${DIR_PATH}/groupby_${mask}_${mkl_list[$bin]}${SUFFIX}.log $tag
		done
	done
	teardown

	if [ "$mask" = "1" ] && want_ml20m; then
		build true false ${mkl_bins[@]}
		for bin in ${mkl_bins[@]}; do
			run_one $bin ml20m \
				${DIR_PATH}/groupby_${mask}_${mkl_list[$bin]}${SUFFIX}.log "${tag}_wi"
		done
		teardown
	fi

done
done


report_timeouts

echo
ls -la ${DIR_PATH}/*sp2m*.log 2> /dev/null

end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date
