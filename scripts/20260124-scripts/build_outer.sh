# mxc

date
start_time=$(date +%s)


CUR_DIR=$(pwd)
BIN_DIR=${CUR_DIR}/../../bin
mkdir ${BIN_DIR}


WORKSPACE_DIR=${CUR_DIR}/../../..
:<<"COMMENT"
# echo ${WORKSPACE_DIR}
# ls ${WORKSPACE_DIR}
tar -xzvf outerspgemm.tar.gz
COMMENT
cd ${WORKSPACE_DIR}/outerspgemm
make clean
make spgemm
 
cd ${BIN_DIR}
ln -s ${WORKSPACE_DIR}/outerspgemm/bin/OuterSpGEMM_hw outerspgemm



end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date