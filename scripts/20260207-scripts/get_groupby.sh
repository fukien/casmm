#!/bin/bash

date
start_time=$(date +%s)

CUR_DIR=$(pwd)


mkdir -p ../../dataset/groupby-raw

bash get_grouplens.sh
python sort_tsv2csv.py

cd ../..
bash revitalize.sh


datasets=(
	"bkgn"
	"ml20m"
)

for dataset in ${datasets[@]}; do
	./bin/csv2bin groupby $dataset
done

bash clean.sh


cd ${CUR_DIR}


end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date
