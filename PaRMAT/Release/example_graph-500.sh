date
start_time=$(date +%s)

./PaRMAT -nVertices 100000 -nEdges 1000000 -output g500_unsorted.txt -a 0.57 -b 0.19 -c 0.19 -threads 32 -noEdgeToSelf -noDuplicateEdges -undirected -memUsage 0.4
./PaRMAT -nVertices 100000 -nEdges 1000000 -output g500_sorted.txt -a 0.57 -b 0.19 -c 0.19 -threads 32 -sorted -noEdgeToSelf -noDuplicateEdges -undirected -memUsage 0.4

end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date
