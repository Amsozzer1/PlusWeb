CXX = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra -g

# Build directory
BUILD_DIR = build

GTEST_INCLUDE = -I/opt/homebrew/Cellar/googletest/1.17.0/include
GTEST_LIBS = -L/opt/homebrew/Cellar/googletest/1.17.0/lib -lgtest -lgtest_main -pthread

TARGET = myprogram

SOURCES = main.cpp src/HttpServer.cpp src/utils.cpp src/HttpRequest.cpp src/HttpResponse.cpp src/RouteRegistry.cpp src/trie.cpp src/RoutingBase.cpp src/Router.cpp src/HttpBody.cpp
OBJECTS = $(SOURCES:%.cpp=$(BUILD_DIR)/%.o)

TEST_TARGET = test_runner
TEST_SOURCES = tests/test_utils.cpp tests/test_RouteRegistry.cpp tests/test_HttpRequest.cpp tests/test_HttpResponse.cpp
TEST_LIB_SOURCES = src/utils.cpp src/RouteRegistry.cpp src/HttpRequest.cpp src/HttpResponse.cpp src/trie.cpp src/HttpBody.cpp
TEST_OBJECTS = $(TEST_SOURCES:%.cpp=$(BUILD_DIR)/%.o) $(TEST_LIB_SOURCES:%.cpp=$(BUILD_DIR)/%.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET)

# Create build directory and compile
$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -Iinclude -c $< -o $@

$(BUILD_DIR)/tests/%.o: tests/%.cpp | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -Iinclude $(GTEST_INCLUDE) -c $< -o $@

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJECTS)
	$(CXX) $(TEST_OBJECTS) $(GTEST_LIBS) -o $(TEST_TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(TEST_TARGET)

check-gtest:
	@echo "Testing Google Test compilation..."
	@echo '#include <gtest/gtest.h>' > /tmp/gtest_test.cpp
	@echo 'int main(){return 0;}' >> /tmp/gtest_test.cpp
	@$(CXX) -std=c++17 $(GTEST_INCLUDE) $(GTEST_LIBS) /tmp/gtest_test.cpp -o /tmp/gtest_test && echo " Google Test working" || echo " Google Test not working"
	@rm -f /tmp/gtest_test.cpp /tmp/gtest_test

.PHONY: all clean test run check-gtest