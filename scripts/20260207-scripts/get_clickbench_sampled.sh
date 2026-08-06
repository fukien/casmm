#!/bin/bash

date
start_time=$(date +%s)

CUR_DIR=$(pwd)

echo "=== Step 1: Sample clickbench dataset (uniform) ==="
python sample_clickbench_uniform.py

echo "=== Step 2: Sample clickbench dataset (skew) ==="
python sample_clickbench_skew.py

echo "=== Step 3: Build csv2bin ==="
cd ../..
bash revitalize.sh

echo "=== Step 4: Generate org.bin and trs.bin ==="
./bin/csv2bin groupby chbc_s_uni
./bin/csv2bin groupby chbc_s_skew

echo "=== Step 5: Clean up ==="
bash clean.sh

cd ${CUR_DIR}

end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date
