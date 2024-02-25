#!/bin/bash

# Output file for CPU usage and temperature
CPU_TEMP_FILE="cpu_temperature_output.csv"

# Output file for thread info
THREAD_INFO_FILE="timer_thread_info.csv"


collect_thread_info() {
    TIMESTAMP=$(date +"%Y-%m-%d %H:%M:%S")
    echo "Timestamp,PID,TID,PSR,CPU %,Memory %,Elapsed Time,Command" >> "$THREAD_INFO_FILE"
    ps -eLo pid,tid,psr,%cpu,%mem,etime,args | grep "[t]imer" | while read -r line; do
        echo "$TIMESTAMP,$line" >> "$THREAD_INFO_FILE"
    done
}

get_overall_stats() {
    TIMESTAMP=$(date +"%Y-%m-%d %H:%M:%S")
    CPU_USAGE=$(top -bn1 | grep "Cpu(s)" | \
                 sed "s/.*, *\([0-9.]*\)%* id.*/\1/" | \
                 awk '{print 100 - $1}')
    TEMPERATURE=$(vcgencmd measure_temp | awk -F "=" '{print $2}' | sed 's/..$//')
    
    echo "Timestamp,CPU %,Temperature (C)" >> "$CPU_TEMP_FILE"
    echo "$TIMESTAMP,$CPU_USAGE,$TEMPERATURE" >> "$CPU_TEMP_FILE"
    echo "Timestamp: $TIMESTAMP, CPU %: $CPU_USAGE, Temperature: $TEMPERATURE" 
}


STATS_INTERVAL=300  # 5 minutes
TOTAL_STATS_TIME=15300  # 4 hours and 15 minutes
SAMPLES=$((TOTAL_STATS_TIME / STATS_INTERVAL))

echo "Collecting CPU stats, temperature, and thread info every $((STATS_INTERVAL/60)) minutes for $((TOTAL_STATS_TIME/60)) minutes..."
echo "Outputting data to $CPU_TEMP_FILE and $THREAD_INFO_FILE"

# Clear previous files
> "$CPU_TEMP_FILE"
> "$THREAD_INFO_FILE"

for ((i=1; i<=SAMPLES; i++))
do

    get_overall_stats
    

    collect_thread_info

    sleep $STATS_INTERVAL  
done

echo "CPU usage and temperature data saved to $CPU_TEMP_FILE"
echo "Thread info saved to $THREAD_INFO_FILE"
