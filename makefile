# Compiler and flags
CC = mpicc
CFLAGS = -fopenmp -Wall -O3
LIBS = -lm
MPI = mpirun

# Target executable
TARGET = integration
SRC = integration.c

# Variables for quick testing (can be overridden via command line)
FUNC = 1
P = 4
T = 2
MODE = 2
TOL = 1e-8

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LIBS)

# General run command: make run P=4 T=2 MODE=2 FUNC=1 TOL=1e-8
run: $(TARGET)
	OMP_NUM_THREADS=$(T) $(MPI) -np $(P) ./$(TARGET) $(FUNC) $(MODE) $(TOL)

# Mode 0: Serial Baseline [cite: 66]
test-serial: $(TARGET)
	$(MPI) -np 1 ./$(TARGET) $(FUNC) 0 $(TOL)

# Mode 1: MPI Dynamic (P=4) 
test-dynamic: $(TARGET)
	$(MPI) -np 4 ./$(TARGET) $(FUNC) 1 $(TOL)

# Mode 2: Hybrid (P=2, T=4) 
test-hybrid: $(TARGET)
	OMP_NUM_THREADS=4 $(MPI) -np 2 ./$(TARGET) $(FUNC) 2 $(TOL)

# Performance measurement helper
experiments: $(TARGET)
	@echo "Running MPI Dynamic (P=1, 2, 4, 8, 16)..."
	for p in 1 2 4 8 16 ; do $(MPI) -np $$p ./$(TARGET) $(FUNC) 1 $(TOL) ; done
	@echo "Running Hybrid (P=2 T=4, P=4 T=2, P=4 T=4)..."
	OMP_NUM_THREADS=4 $(MPI) -np 2 ./$(TARGET) $(FUNC) 2 $(TOL)
	OMP_NUM_THREADS=2 $(MPI) -np 4 ./$(TARGET) $(FUNC) 2 $(TOL)
	OMP_NUM_THREADS=4 $(MPI) -np 4 ./$(TARGET) $(FUNC) 2 $(TOL)

clean:
	rm -f $(TARGET)