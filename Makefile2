# Human Chess Engine - Makefile
# Built with C++17 for performance

CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -pedantic
LDFLAGS = -pthread

# Directories
SRC_DIR = src
BUILD_DIR = build

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.cpp $(SRC_DIR)/*/*.cpp)
HEADERS = $(wildcard $(SRC_DIR)/*.hpp $(SRC_DIR)/*/*.hpp)

# Object files
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))

# Executable
TARGET = human-chess-engine

# Default target
all: $(TARGET)

# Link
$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^

# Compile
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Debug build
debug: CXXFLAGS += -g -DDEBUG
debug: $(TARGET)

# Clean
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# Run tests
test: $(TARGET)
	./$(TARGET)

# Install
PREFIX = /usr/local
install: $(TARGET)
	install -d $(PREFIX)/bin
	install $(TARGET) $(PREFIX)/bin/

uninstall:
	rm -f $(PREFIX)/bin/$(TARGET)

.PHONY: all clean debug test install uninstall
