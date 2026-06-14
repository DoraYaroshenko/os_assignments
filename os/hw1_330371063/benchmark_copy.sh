#!/bin/bash
gcc -O3 -Wall -std=c11 copy.c -o copy
echo "Starting benchmark..."

if [[ $# -ne 3 ]]; then
    echo "Invalid number of arguments" >&2
    exit 1
fi

if [[ ! -f "copy" ]]; then
    echo "Copy program does not exist" >&2
    exit 1
fi

if ! [[ -f $1 ]]; then
    echo "Input file does not exist" >&2
    exit 1
fi

required_file_size=$((100 * 1024 * 1024))
file_size=$(stat -c%s $1)

if [[ $file_size -lt $required_file_size ]]; then
    echo "Input file is too small" >&2
    exit 1
fi

echo "Buffer Size,Time (seconds)" > results.csv
echo "-----------,--------------" >> results.csv

echo "Input file: $1 ($file_size bytes)"
echo "Testing buffer sizes: $3"

IFS=',' read -ra SIZES <<< "$3"
SIZES_length=${#SIZES[@]}

succeeded_tests=0
input_file="$1"
output_file="$2"

for (( i = 0; i < SIZES_length; i++ )); do
    curr_size=${SIZES[$i]}
    echo "[$((i+1))/$SIZES_length] Testing buffer size: $curr_size bytes"
    echo "Clearing cache..."
    sync; echo 3 | sudo tee /proc/sys/vm/drop_caches > /dev/null 2>&1
    time_sec=$({ /usr/bin/time -f "%e" ./copy "$1" "$2" "$curr_size"; } 2>&1 )
    exit_code=$?
    if [[ $exit_code -eq 0 ]]; then
        echo "Running copy... Done (${time_sec}s)"
        echo "$curr_size,$time_sec" >> results.csv
        ((succeeded_tests++))
    else
        echo "Running copy... Failed"
        echo "Error: Copy program failed for buffer size $curr_size"
    fi
    rm -f $2
done

echo "Benchmark complete!"
echo "Results saved to: results.csv"
echo "Summary:"
while IFS=',' read -r buffer_size time; do
    printf "%-20s %s\n" "$buffer_size" "$time"
done < results.csv
echo "Tests completed: $succeeded_tests/$SIZES_length"