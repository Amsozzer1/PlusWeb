# Compiler and flags
CXX = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra -g

# Target executable name
TARGET = myprogram

# Source files
SOURCES = main.cpp HttpServer.cpp utils.cpp HttpRequest.cpp HttpResponse.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# Default target
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET)

# Compile source files to object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# run
run: $(TARGET)
	 ./$(TARGET)
# Clean up generated files
clean:
	rm -f $(OBJECTS) $(TARGET)

# Prevent make from confusing targets with filenames
.PHONY: all clean