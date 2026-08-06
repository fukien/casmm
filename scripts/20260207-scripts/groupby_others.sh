
date
start_time=$(date +%s)

source ./common.inc.sh

declare -A others_list=(
	["hashspgemm_jp_2p"]="hash_jp_2p"
	["hashspgemm_jp_2p_sat"]="hash_jp_2p_sat"
	["nphj_sc_aggotfoa_jp"]="nphj_sc_aggotfoa_jp"
	["phj_rdx_bc_agg_jp"]="phj_rdx_bc_agg_jp"
)
others_bins=(
	"hashspgemm_jp_2p"
	"hashspgemm_jp_2p_sat"
	"nphj_sc_aggotfoa_jp"
	"phj_rdx_bc_agg_jp"
)

banner


for mask in $MASKS; do
set_mask $mask
for nthread in $THREADS; do
set_threads $nthread

	for bin in ${others_bins[@]}; do
		rm -f ${DIR_PATH}/groupby_${mask}_${others_list[$bin]}${SUFFIX}.log
	done

	tag="m${mask}_${nthread}t"

	build $DEFAULT_WI false ${others_bins[@]}
	for dataset in ${groupby_dataset_list[@]}; do
		if [ "$mask" = "1" ] && [ "$dataset" = "ml20m" ]; then continue; fi
		for bin in ${others_bins[@]}; do
			run_one $bin $dataset \
				${DIR_PATH}/groupby_${mask}_${others_list[$bin]}${SUFFIX}.log $tag
		done
	done
	teardown

	if [ "$mask" = "1" ] && want_ml20m; then
		build true false ${others_bins[@]}
		for bin in ${others_bins[@]}; do
			run_one $bin ml20m \
				${DIR_PATH}/groupby_${mask}_${others_list[$bin]}${SUFFIX}.log "${tag}_wi"
		done
		teardown
	fi

done
done


report_timeouts

echo
ls -la ${DIR_PATH}/*.log 2> /dev/null

end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date
