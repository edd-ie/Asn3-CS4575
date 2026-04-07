# Hybrid MPI+OpenMP Adaptive Numerical Integration

This project evaluates the performance of an **Adaptive Simpson’s Rule** implementation using a hybrid parallel approach that combines **MPI** (Message Passing Interface) for distributed memory and **OpenMP** for shared memory task-based parallelism.

## Features
- **Three Modes of Operation**:
  - **Mode 0**: Serial baseline execution.
  - **Mode 1**: MPI Dynamic Load Balancing using a Master-Worker architecture.
  - **Mode 2**: Hybrid MPI+OpenMP approach using static MPI partitioning and OpenMP recursive tasks.
- **Adaptive Precision**: Intervals are dynamically split until the required tolerance is met.
- **Performance Analysis**: Includes automated scripts for measuring speedup and efficiency across various worker configurations.

## Compilation
The program requires an MPI wrapper (e.g., Open MPI) and a compiler supporting OpenMP.

```bash
# Using the provided Makefile
make

# Manual compilation
mpicc -O3 -fopenmp integration.c -o integration -lm
```

**Flags:**
```
-O3: Aggressive code optimization for numerical performance
-fopenmp: Enables OpenMP pragma support for Mode 2.
-lm: Links the standard math library.
```
## Execution
The executable accepts four command-line arguments:
```bash
mpirun -np [P] ./integration [func_id] [mode] [tol]
```

| Argument | Description|
|------------|------------------|
| P  | Number of MPI processes|
| func_id | 0 (Simple), 1 (Spike), or 2 (Oscillatory)|
| mode | 0 (Serial), 1 (MPI Dynamic), 2 (Hybrid MPI+OpenMP)|
|tol | Target  error tolerance (e.g., 1e-8)|


**Run Examples:**
```Bash
# Run Mode 1 (MPI Dynamic) with 4 processes
mpirun -np 4 ./integration 1 1 1e-8

# Run Mode 2 (Hybrid) with 2 processes and 4 threads each
OMP_NUM_THREADS=4 mpirun -np 2 ./integration 1 2 1e-8
```

## Performance Results (Function 1)
Benchmarks conducted on an Intel i5-8250U (4 Cores / 8 Threads) reveal significant parallel overhead for small workloads:

|Mode|Processes (P)|Threads (T)|Workers (PxT)| Speedup |Efficiency |
|---|---|---|---|---|---|
|Serial | 1| 1|1|1.00|100%|
|MPI|4|1|4|0.027|0.6%|
|Hybrid|2|4|8|0.034|0.4%|
|Hybrid|4|4|16|0.0006|0.003%|

Note: *Efficiency drops significantly at higher worker counts due to the small computational granularity of Function 1 relative to MPI/OpenMP orchestration costs.*