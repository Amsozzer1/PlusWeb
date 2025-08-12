# Compiler and flags
CXX = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra -g

# Google Test paths
GTEST_INCLUDE = -I/opt/homebrew/Cellar/googletest/1.17.0/include
GTEST_LIBS = -L/opt/homebrew/Cellar/googletest/1.17.0/lib -lgtest -lgtest_main -pthread

# Target executable name
TARGET = myprogram

# Source files
SOURCES = main.cpp src/HttpServer.cpp src/utils.cpp src/HttpRequest.cpp src/HttpResponse.cpp src/RouteRegistry.cpp src/trie.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# Test configuration
TEST_TARGET = test_runner
TEST_SOURCES = tests/test_utils.cpp tests/test_RouteRegistry.cpp tests/test_HttpRequest.cpp tests/test_HttpResponse.cpp
TEST_LIB_SOURCES = src/utils.cpp src/RouteRegistry.cpp src/HttpRequest.cpp src/HttpResponse.cpp src/trie.cpp
TEST_OBJECTS = $(TEST_SOURCES:.cpp=.o) $(TEST_LIB_SOURCES:.cpp=.o)

# Default target
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET)

# Compile source files to object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -Iinclude -c $< -o $@

# Test target
test: $(TEST_TARGET)
	./$(TEST_TARGET)

# Build test executable
$(TEST_TARGET): $(TEST_OBJECTS)
	$(CXX) $(TEST_OBJECTS) $(GTEST_LIBS) -o $(TEST_TARGET)

# Build test objects with Google Test includes
tests/%.o: tests/%.cpp
	$(CXX) $(CXXFLAGS) -Iinclude $(GTEST_INCLUDE) -c $< -o $@

# run
run: $(TARGET)
	./$(TARGET)

# Clean up generated files
clean:
	rm -f $(OBJECTS) $(TARGET) $(TEST_OBJECTS) $(TEST_TARGET)

# Check if Google Test is working
# Check if Google Test is working
# Check if Google Test is working
# Check if Google Test is working
check-gtest:
	@echo "Testing Google Test compilation..."
	@echo '#include <gtest/gtest.h>' > /tmp/gtest_test.cpp
	@echo 'int main(){return 0;}' >> /tmp/gtest_test.cpp
	@$(CXX) -std=c++17 $(GTEST_INCLUDE) $(GTEST_LIBS) /tmp/gtest_test.cpp -o /tmp/gtest_test && echo "✅ Google Test working" || echo "❌ Google Test not working"
	@rm -f /tmp/gtest_test.cpp /tmp/gtest_test

# Prevent make from confusing targets with filenames
.PHONY: all clean test run check-gtest