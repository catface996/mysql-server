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
              ../../src/sbt_file.cc \
              ../../src/sbt_share.cc \
              ../../src/sbt_raii.cc

# Libraries (minimal set for standalone test)
LIBS = -L$(BUILD_DIR)/archive_output_directory \
       -L$(BUILD_DIR)/library_output_directory \
       -lmysys -lstrings -lmytime

# Test executables
test_file_format: test_file_format.cc $(SBT_SOURCES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)

comprehensive_file_format_test: comprehensive_file_format_test.cc $(SBT_SOURCES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)

test_file_operations_standalone: test_file_operations_standalone.cc $(SBT_SOURCES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)

test_file_operations_simple: test_file_operations_simple.cc
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)

test_file_basic: test_file_basic.cc
	$(CXX) -std=c++17 -g -O0 -Wall -Wextra -o $@ $^

test_sbt_share_standalone: test_sbt_share_standalone.cc
	$(CXX) -std=c++17 -g -O0 -Wall -Wextra -o $@ $^

test_share_resource_management: test_share_resource_management.cc $(SBT_SOURCES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)

test_task_4_2_verification: test_task_4_2_verification.cc
	$(CXX) -std=c++17 -g -O0 -Wall -Wextra -o $@ $^

test_task_6_2_verification: test_task_6_2_verification.cc
	$(CXX) -std=c++17 -g -O0 -Wall -Wextra -o $@ $^

test_plugin_interface_complete: test_plugin_interface_complete.cc
	$(CXX) -std=c++17 -g -O0 -Wall -Wextra -o $@ $^

test_task_5_1_verification: test_task_5_1_verification.cc
	$(CXX) -std=c++17 -g -O0 -Wall -Wextra -o $@ $^

test_task_5_4_verification: test_task_5_4_verification.cc
	$(CXX) -std=c++17 -g -O0 -Wall -Wextra -o $@ $^

test_task_5_6_verification: test_task_5_6_verification.cc
	$(CXX) -std=c++17 -g -O0 -Wall -Wextra -o $@ $^

test_task_5_7_verification: test_task_5_7_verification.cc
	$(CXX) -std=c++17 -g -O0 -Wall -Wextra -o $@ $^

test_full_table_scan_standalone: test_full_table_scan_standalone.cc
	$(CXX) -std=c++17 -g -O0 -Wall -Wextra -o $@ $^

test_task_6_1_verification: test_task_6_1_verification.cc
	$(CXX) -std=c++17 -g -O0 -Wall -Wextra -o $@ $^

# Exception safety and resource management tests
test_exception_safety: test_exception_safety.cc $(SBT_SOURCES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LIBS) -pthread

test_memory_leaks: test_memory_leaks.cc $(SBT_SOURCES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LIBS) -pthread

test_resource_stress: test_resource_stress.cc $(SBT_SOURCES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LIBS) -pthread

# Simple RAII tests (no MySQL dependencies)
test_raii_simple: test_raii_simple.cc
	$(CXX) -std=c++17 -g -O0 -Wall -Wextra -o $@ $^ -pthread

# Task 8.1: Basic functionality tests (standalone)
test_basic_functionality_standalone: test_basic_functionality_standalone.cc
	$(CXX) -std=c++17 -g -O0 -Wall -Wextra -o $@ $^

# Task 8.1: Basic functionality tests (simplified version)
test_basic_functionality_simple: test_basic_functionality_simple.cc
	$(CXX) -std=c++17 -g -O0 -Wall -Wextra -o $@ $^

# Run all file format tests
test-file-format: test_file_format comprehensive_file_format_test
	@echo "=== Running Basic File Format Test ==="
	./test_file_format
	@echo ""
	@echo "=== Running Comprehensive File Format Test ==="
	./comprehensive_file_format_test

# Run file operations tests
test-file-operations: test_file_basic
	@echo "=== Running File Operations Test (Basic) ==="
	./test_file_basic

# Run share management tests
test-share-management: test_sbt_share_standalone test_task_4_2_verification
	@echo "=== Running SBT Share Standalone Test ==="
	./test_sbt_share_standalone
	@echo ""
	@echo "=== Running Task 4.2 Verification Test ==="
	./test_task_4_2_verification

# Run exception safety and resource management tests
test-exception-safety: test_exception_safety test_memory_leaks test_resource_stress
	@echo "=== Running Exception Safety Tests ==="
	./test_exception_safety
	@echo ""
	@echo "=== Running Memory Leak Detection Tests ==="
	./test_memory_leaks
	@echo ""
	@echo "=== Running Resource Management Stress Tests ==="
	./test_resource_stress

# Run Task 8.1: Basic functionality tests
test-basic-functionality: test_basic_functionality_simple
	@echo "=== Running Task 8.1: Basic Functionality Tests (Simplified) ==="
	./test_basic_functionality_simple

clean:
	rm -f test_file_format comprehensive_file_format_test test_file_operations_standalone test_file_operations_simple test_file_basic test_sbt_share_standalone test_share_resource_management test_task_4_2_verification test_task_5_1_verification test_task_5_4_verification test_task_5_6_verification test_task_5_7_verification test_full_table_scan_standalone test_task_6_1_verification test_exception_safety test_memory_leaks test_resource_stress test_raii_simple test_basic_functionality_standalone test_basic_functionality_simple

.PHONY: clean test-file-format test-file-operations test-share-management test-exception-safety test-basic-functionality