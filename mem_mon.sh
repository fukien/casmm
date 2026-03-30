#!/bin/bash
PID=$1
DATASET=$2
INTERVAL=0.5  # seconds

# Overall RSS values (in kB)
SUM=0
COUNT=0
MAX=0

# NUMA node memory values (we use floats, assumed already in MB from numastat)
SUM0=0.0
SUM1=0.0
SUM2=0.0
MAX0=0.0
MAX1=0.0
MAX2=0.0

while kill -0 $PID 2>/dev/null; do
	# Get overall RSS from /proc/<PID>/status (in kB)
	RSS=$(grep VmRSS /proc/$PID/status | awk '{print $2}')
	
	# Update overall sum and count for average calculation (in kB)
	SUM=$((SUM + RSS))
	COUNT=$((COUNT + 1))
	
	# Update overall peak RSS if current sample is higher
	if [ "$RSS" -gt "$MAX" ]; then
		MAX=$RSS
	fi

	# Get per-NUMA node memory usage via numastat.
	NUMA_TOTAL=$(numastat -p $PID 2>/dev/null | grep -i '^total')
	if [ -n "$NUMA_TOTAL" ]; then
		# Extract NUMA values (assumes whitespace-separated columns)
		NODE0=$(echo "$NUMA_TOTAL" | awk '{print $2}')
		NODE1=$(echo "$NUMA_TOTAL" | awk '{print $3}')
		NODE2=$(echo "$NUMA_TOTAL" | awk '{print $4}')
		
		# Update sums using awk for float addition
		SUM0=$(awk -v s="$SUM0" -v n="$NODE0" 'BEGIN {printf "%.2f", s+n}')
		SUM1=$(awk -v s="$SUM1" -v n="$NODE1" 'BEGIN {printf "%.2f", s+n}')
		SUM2=$(awk -v s="$SUM2" -v n="$NODE2" 'BEGIN {printf "%.2f", s+n}')
		
		# Update maximum for each NUMA node
		comp0=$(awk -v n="$NODE0" -v m="$MAX0" 'BEGIN {print (n > m) ? 1 : 0}')
		if [ "$comp0" -eq 1 ]; then
			MAX0="$NODE0"
		fi
		
		comp1=$(awk -v n="$NODE1" -v m="$MAX1" 'BEGIN {print (n > m) ? 1 : 0}')
		if [ "$comp1" -eq 1 ]; then
			MAX1="$NODE1"
		fi
		
		comp2=$(awk -v n="$NODE2" -v m="$MAX2" 'BEGIN {print (n > m) ? 1 : 0}')
		if [ "$comp2" -eq 1 ]; then
			MAX2="$NODE2"
		fi
	fi

	sleep $INTERVAL
done

if [ $COUNT -gt 0 ]; then
	# Compute overall average RSS (in kB)
	AVG=$((SUM / COUNT))
	
	# Compute per-NUMA node averages using awk for float division
	AVG0=$(awk -v s="$SUM0" -v c="$COUNT" 'BEGIN {printf "%.2f", s/c}')
	AVG1=$(awk -v s="$SUM1" -v c="$COUNT" 'BEGIN {printf "%.2f", s/c}')
	AVG2=$(awk -v s="$SUM2" -v c="$COUNT" 'BEGIN {printf "%.2f", s/c}')
	
	# Convert overall values from kB to MB (1 MB = 1024 kB)
	TOTAL_AVG_MB=$(awk -v avg="$AVG" 'BEGIN {printf "%.2f", avg/1024}')
	TOTAL_PEAK_MB=$(awk -v max="$MAX" 'BEGIN {printf "%.2f", max/1024}')
	
	# Print dataset, overall (converted) values, and NUMA node stats in MB (tab-separated)
	printf "MEMORY_CONSUMPTION: %s\tTOTAL_AVG: %s\tTOTAL_PEAK: %s\tNUMA0_AVG: %s\tNUMA0_PEAK: %s\tNUMA1_AVG: %s\tNUMA1_PEAK: %s\tNUMA2_AVG: %s\tNUMA2_PEAK: %s\n" \
		"$DATASET" "$TOTAL_AVG_MB" "$TOTAL_PEAK_MB" "$AVG0" "$MAX0" "$AVG1" "$MAX1" "$AVG2" "$MAX2"
	exit 0   # Terminate immediately after printing
else
	echo "No data collected."
	exit 1   # End with an error code if no data was collected
fi
