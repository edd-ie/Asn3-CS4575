# Compiler and flags
CC = mpicc
CFLAGS = -fopenmp -Wall -O3
LIBS = -lm
MPI = mpirun -np

# Target executable
TARGET = integration
SRC = integration.c
BUILD = build/src

# Nodes
func_id = 0
P = 1
mode = 0
tol = 1e-6

# Default target
all: $(TARGET)

# Build target
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LIBS) -o $(TARGET)

# mpirun -np P integration func_id mode tol 
# Run target
run0: $(TARGET)
	$(MPI) 1 $(TARGET) $(func_id) 0 $(tol)

run1: $(TARGET)
	$(MPI) 4 $(TARGET) $(func_id) 1 $(tol)

run2: $(TARGET)
	$(MPI) 4 $(TARGET) $(func_id) 2 $(tol)

memcheck: $(TARGET)
	@echo -e "Verifying RAII cleanup."
	$(MPI) 1 valgrind --leak-check=full --show-leak-kinds=all -s ./$(TARGET) $(func_id) 0 $(tol)

# Clean target
clean:
	rm -f $(TARGET)
