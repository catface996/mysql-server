# Simple Makefile for SBT standalone tests

# Build directory
BUILD_DIR = ../../../../build

# Include paths
INCLUDES = -I../../include \
           -I../../../../include \
           -I../../../../sql \
           -I$(BUILD_DIR) \
           -I$(BUILD_DIR)/include

# Compiler flags
CXXFLAGS = -std=c++20 -DMYSQL_SERVER -g -O0 -Wall -Wextra

# Source files
SBT_SOURCES = ../../src/sbt_common.cc \
              ../../src/sbt_tree.cc \
              ../../src/sbt_file.cc

# Libraries (minimal set for standalone test)
LIBS = -L$(BUILD_DIR)/archive_output_directory \
       -L$(BUILD_DIR)/library_output_directory \
       -lmysys -lstrings -lmytime

# Test executables
test_file_format: test_file_format.cc $(SBT_SOURCES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)

comprehensive_file_format_test: comprehensive_file_format_test.cc $(SBT_SOURCES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)

# Run all file format tests
test-file-format: test_file_format comprehensive_file_format_test
	@echo "=== Running Basic File Format Test ==="
	./test_file_format
	@echo ""
	@echo "=== Running Comprehensive File Format Test ==="
	./comprehensive_file_format_test

clean:
	rm -f test_file_format comprehensive_file_format_test

.PHONY: clean test-file-format