#!/bin/bash

date
start_time=$(date +%s)

CUR_DIR=$(pwd)

mkdir -p ../../dataset/groupby-raw
cd ../../dataset/groupby-raw

wget https://files.grouplens.org/datasets/book-genome/book-genome.zip
mkdir -p bkgn
unzip book-genome.zip -d bkgn
wget https://files.grouplens.org/datasets/movielens/ml-20m.zip
mkdir -p ml20m
unzip ml-20m.zip -d ml20m

cd ${CUR_DIR}
python bkgn_json2tsv.py
python csv2tsv_grouplens.py


end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date


