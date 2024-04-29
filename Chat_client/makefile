# Compiler and flags
CC = g++
CFLAGS = -std=c++20 -Wall

# Source files
SRC = main.cpp clientTCP.cpp clientUDP.cpp
# Object files
OBJ = $(SRC:.cpp=.o)
# Executable name
EXEC = ipk24chat-client

# Default target
all: $(EXEC)

# Compile source files into object files
%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

# Link object files into executable
$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

# Clean up intermediate object files and executable
clean:
	rm -f $(OBJ) $(EXEC)
