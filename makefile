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

# Mode 0: Serial Baseline [cite: 66, 67]
test-serial: $(TARGET)
	$(MPI) -np 1 ./$(TARGET) $(FUNC) 0 $(TOL)

# Mode 1: MPI Dynamic (P=4) [cite: 75, 76]
test-dynamic: $(TARGET)
	$(MPI) -np 4 ./$(TARGET) $(FUNC) 1 $(TOL)

# Mode 2: Hybrid (P=2, T=4) [cite: 84, 85]
test-hybrid: $(TARGET)
	export OMP_NUM_THREADS=4; $(MPI) -np 2 ./$(TARGET) $(FUNC) 2 $(TOL)

# Corrected Full-Suite to meet Section 8 requirements [cite: 111, 112, 113, 115]
full-suite: $(TARGET)
	@echo "--- Running Required MPI Configurations (Mode 1) ---"
	@for p in 1 2 4 8 16 ; do \
		echo "Running P=$$p..."; \
		$(MPI) -np $$p ./$(TARGET) 1 1 1e-8 ; \
	done
	@echo "--- Running Required Hybrid Configurations (Mode 2) ---"
	@echo "Running (P=2, T=4)..."
	@export OMP_NUM_THREADS=4; $(MPI) -np 2 ./$(TARGET) 1 2 1e-8
	@echo "Running (P=4, T=2)..."
	@export OMP_NUM_THREADS=2; $(MPI) -np 4 ./$(TARGET) 1 2 1e-8
	@echo "Running (P=4, T=4)..."
	@export OMP_NUM_THREADS=4; $(MPI) -np 4 ./$(TARGET) 1 2 1e-8

clean:
	rm -f $(TARGET)