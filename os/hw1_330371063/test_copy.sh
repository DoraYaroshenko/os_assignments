#!/bin/bash
gcc -O3 -Wall -std=c11 copy.c -o copy
echo "Copy Program Test Suite"
echo "======================="
echo "Setting up test environment..."

path="test_files"
mkdir -p "$path"

echo "Test directory: $path/"

test_cases=("0" "10" "1024" "1048576" "10485760" "5242880")
buffer_sizes=("512" "64" "1024" "4096" "8192" "4096")
test_names=("Empty file" "Small file" "Exact buffer size" "Medium file" "Large file" "Binary data")
correctness_tests_passed=0
error_tests_passed=0
start_time=$(date +%s)

echo "Running correctness tests..."

for i in {0..5}; do
    echo "[TEST $((i+1))] ${test_names[$i]} (${test_cases[$i]} bytes, buffer=${buffer_sizes[$i]})"
    if [ "$i" -eq 0 ]; then
        dd if=/dev/zero of="$path/input" count=0 > /dev/null 2>&1
    elif [ "$i" -lt 5 ]; then
        dd if=/dev/zero of="$path/input" bs=${test_cases[$i]} count=1 > /dev/null 2>&1
    else
        dd if=/dev/urandom of="$path/input" bs=${test_cases[$i-1]} count=1 > /dev/null 2>&1
    fi
    echo "Creating test file... Done"
    ./copy "$path"/input "$path"/output ${buffer_sizes[$i]} > /dev/null 2>&1
    echo "Creating copy... Done"
    if [ $? -ne 0 ]; then
        echo "Failed"
    fi
    checksum1=$(md5sum "$path"/input | cut -d' ' -f1)
    checksum2=$(md5sum "$path"/output | cut -d' ' -f1)
    if [ "$checksum1" = "$checksum2" ]; then
        echo "Verifying checksums... PASS"
        ((correctness_tests_passed++))
    else
        echo "Verifying checksums... FAIL"
    fi
    rm -rf "$path"/*
done

echo "Running error handling tests..."

echo "[ERROR TEST 1/3] Non-existent input file"
echo "Expected: Failure"
./copy "$path"/input "$path"/output 1024 > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "Result: Program failed as expected"
    ((error_tests_passed++))
else
    echo "Result: Program succeeded, error"
fi

echo "[ERROR TEST 2/3] Output file already exists"
echo "Expected: Failure"
dd if=/dev/zero of="$path/input" bs=10 count=1 > /dev/null 2>&1
dd if=/dev/zero of="$path/output" bs=10 count=1 > /dev/null 2>&1
./copy "$path"/input "$path"/output 1024 > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "Result: Program failed as expected"
    ((error_tests_passed++))
else
    echo "Result: Program succeeded, error"
fi

echo "[ERROR TEST 3/3] Insufficient arguments"
echo "Expected: Failure"
./copy input output > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "Result: Program failed as expected"
    ((error_tests_passed++))
else
    echo "Result: Program succeeded, error"
fi

end=$(date +%s)
elapsed=$((end-start))

echo "Cleaning up test files..."
rm -rf "$path"/

total_passed=$(($correctness_tests_passed+$error_tests_passed))

echo "======================="
echo "Test Summary"
echo "======================="

echo "Correctness Tests: $correctness_tests_passed/6"
echo "Error Handling Tests: $error_tests_passed/3"
echo "Total $total_passed/9"

if [ $total_passed -eq 9 ]; then
    echo "All tests PASSED!"
else
    echo "Some tests failed"
fi
elapsed_sec=$(echo "scale=3; $elapsed / 1000000000" | bc)
echo "Total execution time: $elapsed_sec seconds"