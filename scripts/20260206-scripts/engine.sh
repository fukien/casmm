date
start_time=$(date +%s)

bash groupby_ab.sh > groupby_ab.log 2>&1
bash groupby_others.sh > groupby_others.log 2>&1

end_time=$(date +%s)
duration=$((end_time - start_time))
echo "Duration: $duration seconds"
date