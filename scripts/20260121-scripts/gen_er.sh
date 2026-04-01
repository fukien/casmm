date
start_time=$(date +%s)

THREAD_NUM=8

mkdir -p ../../dataset/rmat/

cd ../..
./revitalize.sh
cd PaRMAT/Release
make clean && make

for scale in 14 15 16 17 18 19 20 21; do
	for edge_factor in 1 2 4 8 16 32 64; do
		num_vertices=$((2**scale))
		num_edges=$((edge_factor * num_vertices))
		./PaRMAT -nVertices "$num_vertices" -nEdges "$num_edges" \
			-output ../../dataset/rmat/er_"$scale"_"$edge_factor".txt \
			-a 0.25 -b 0.25 -c 0.25 -threads "$THREAD_NUM" \
			-sorted -noEdgeToSelf -noDuplicateEdges -undirected -memUsage 0.4
		cd ../..
		./bin/rmat_ones er_"$scale"_"$edge_factor"
		cd PaRMAT/Release
	done
done



end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date
