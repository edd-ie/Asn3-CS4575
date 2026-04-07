# Compiler and flags
CC = mpicc
CFLAGS = -fopenmp -Wall -O3
LIBS = -lm
MPI = mpirun

# Target executable
TARGET = integration
SRC = integration.c

FUNC = 1
P = 4
T = 2
MODE = 2
TOL = 1e-8

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LIBS)

# Mode 0: Serial Baseline 
serial: $(TARGET)
	$(MPI) -np 1 ./$(TARGET) $(FUNC) 0 $(TOL)

# Mode 1: MPI Dynamic (P=4) 
mpi: $(TARGET)
	$(MPI) -np 4 ./$(TARGET) $(FUNC) 1 $(TOL)

# Mode 2: Hybrid (P=2, T=4)
hybrid: $(TARGET)
	export OMP_NUM_THREADS=4; $(MPI) -np 2 ./$(TARGET) $(FUNC) 2 $(TOL)


clean:
	rm -f $(TARGET)