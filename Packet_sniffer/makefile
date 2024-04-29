# Compiler
CC=g++
# Compiler flags
CFLAGS=-std=c++20 -c
# Linker flags
LDFLAGS= -lpcap 
# Source files
SOURCES=main.cpp sniff.cpp filter.cpp
# Object files
OBJECTS=$(SOURCES:.cpp=.o)
# Executable name
EXECUTABLE=ipk-sniffer

# Compilation rule for .cpp files
.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)
