# Define variables
TARGET = benchmark.exe
SRC = benchmark.cpp
HEADER = hashes.hpp
OBJ = benchmark.obj
CC = cl
CFLAGS = /EHsc /O2 /W3 /std:c++17
LINKER = link
LFLAGS = /OUT:$(TARGET)

# Default rule
all: $(TARGET)

# Rule to build the executable
$(TARGET): $(OBJ)
	$(LINKER) $(LFLAGS) $(OBJ)

# Rule to compile the source files
$(OBJ): $(SRC) $(HEADER)
	$(CC) $(CFLAGS) /c $(SRC)

# Clean rule
clean:
	del $(OBJ) $(TARGET)
