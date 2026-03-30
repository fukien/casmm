for EDGEFACTOR in 1 2 4 8 16 32 64; do
    numactl --physcpubind=0-7 ./bin/GenMatrices_hw gen rmat 23 $EDGEFACTOR 8
done
