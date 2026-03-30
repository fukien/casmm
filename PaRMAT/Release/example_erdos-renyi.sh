date
start_time=$(date +%s)

./PaRMAT -nVertices 100000 -nEdges 1000000 -output er_unsorted.txt -a 0.25 -b 0.25 -c 0.25 -threads 32 -noEdgeToSelf -noDuplicateEdges -undirected -memUsage 0.4
./PaRMAT -nVertices 100000 -nEdges 1000000 -output er_sorted.txt -a 0.25 -b 0.25 -c 0.25 -threads 32 -sorted -noEdgeToSelf -noDuplicateEdges -undirected -memUsage 0.4

end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date
