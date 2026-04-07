#!/bin/bash

# Configuration
EXE="./integration"
FUNC=1
TOL="1e-8"
TIMEOUT="30s"

# Ensure the executable exists
if [ ! -f "$EXE" ]; then
    echo "Error: $EXE not found. Run 'make' first."
    exit 1
fi

echo "=========================================================="
echo "Parallel Computing Assignment 3 Experiments (with Oversubscribe)"
echo "Function: $FUNC, Tolerance: $TOL"
echo "=========================================================="

# 1. Serial Baseline (Mode 0) - Required for Section 6 [cite: 277]
echo -n "Running Serial Baseline (P=1)... "
timeout $TIMEOUT mpirun --oversubscribe -np 1 $EXE $FUNC 0 $TOL | grep "Average Time"

# 2. MPI Dynamic Configurations (Mode 1) [cite: 325]
echo -e "\n--- Running MPI Dynamic (Mode 1) ---"
for p in 2 4 8 16; do
    echo -n "Running P=$p... "
    # Added --oversubscribe to allow P > physical cores
    RESULT=$(timeout $TIMEOUT mpirun --oversubscribe -np $p $EXE $FUNC 1 $TOL | grep "Average Time")
    if [ $? -eq 124 ]; then
        echo "TIMED OUT"
    else
        echo "$RESULT"
    fi
done

# 3. Hybrid Configurations (Mode 2) [cite: 327]
echo -e "\n--- Running Hybrid MPI + OpenMP (Mode 2) ---"
declare -a configs=("2 4" "4 2" "4 4")

for config in "${configs[@]}"; do
    set -- $config
    p=$1
    t=$2
    echo -n "Running P=$p, T=$t... "
    export OMP_NUM_THREADS=$t
    RESULT=$(timeout $TIMEOUT mpirun --oversubscribe -np $p $EXE $FUNC 2 $TOL | grep "Average Time")
    if [ $? -eq 124 ]; then
        echo "TIMED OUT"
    else
        echo "$RESULT"
    fi
done

echo -e "\n=========================================================="
echo "Experiments Complete."