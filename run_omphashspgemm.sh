#!/bin/bash

CORE_RANGE="32-63,96-127"
# CORE_RANGE="0-31,64-95"
NUMACTL="/home/huang/workspace/numactl/numactl"

start_time=$(date +%s)

if [ "$#" -eq 3 ]; then
	export OMP_SCHEDULE="$2,$3"
	echo "$OMP_SCHEDULE"
else
	export OMP_SCHEDULE="dynamic,1"
	echo "$OMP_SCHEDULE"
fi

$NUMACTL --physcpubind=$CORE_RANGE ./bin/omphashspgemm "$1"

end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
