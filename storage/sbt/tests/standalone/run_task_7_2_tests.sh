#!/bin/bash

# SBT Task 7.2 - Exception Safety and Resource Management Tests
# This script runs all tests related to task 7.2 implementation

set -e  # Exit on any error

echo "=== SBT Task 7.2: Exception Safety and Resource Management Tests ==="
echo "=================================================================="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test results tracking
TOTAL_TESTS=0
PASSED_TESTS=0

# Function to run a test and report results
run_test() {
    local test_name="$1"
    local test_command="$2"
    
    echo -e "${BLUE}Running: $test_name${NC}"
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    if eval "$test_command"; then
        echo -e "${GREEN}✓ PASSED: $test_name${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}✗ FAILED: $test_name${NC}"
        echo -e "${RED}❌ Task 7.2 test failed: $test_name${NC}"
        return 1
    fi
    echo ""
}

# Clean up previous builds
echo "Cleaning up previous builds..."
make clean 2>/dev/null || true

echo -e "${YELLOW}Building Task 7.2 tests...${NC}"

# Build all exception safety tests
echo "Building exception safety tests..."
make test_exception_safety
make test_memory_leaks  
make test_resource_stress

echo -e "${GREEN}All tests built successfully!${NC}"
echo ""

# Run Task 7.2 specific tests
echo -e "${YELLOW}=== Running Task 7.2 Exception Safety Tests ===${NC}"

run_test "RAII and Exception Safety Tests" "./test_exception_safety"
run_test "Memory Leak Detection Tests" "./test_memory_leaks"
run_test "Resource Management Stress Tests" "./test_resource_stress"

# Run regression tests to ensure previous functionality still works
echo -e "${YELLOW}=== Running Regression Tests ===${NC}"

# Build and run core functionality tests
if [ -f "test_insertion_standalone.cc" ]; then
    run_test "Core Data Structure Tests (Regression)" "make test_insertion_standalone && ./test_insertion_standalone"
fi

if [ -f "test_deletion_standalone.cc" ]; then
    run_test "Deletion Operations Tests (Regression)" "make test_deletion_standalone && ./test_deletion_standalone"
fi

if [ -f "test_search_traversal_standalone.cc" ]; then
    run_test "Search and Traversal Tests (Regression)" "make test_search_traversal_standalone && ./test_search_traversal_standalone"
fi

if [ -f "verify_file_format.sh" ]; then
    run_test "File Format Tests (Regression)" "./verify_file_format.sh"
fi

if [ -f "verify_serialization.sh" ]; then
    run_test "Serialization Tests (Regression)" "./verify_serialization.sh"
fi

# Final results
echo "=== Task 7.2 Test Results ==="
echo "Total tests: $TOTAL_TESTS"
echo "Passed: $PASSED_TESTS"
echo "Failed: $((TOTAL_TESTS - PASSED_TESTS))"

if [ $PASSED_TESTS -eq $TOTAL_TESTS ]; then
    echo -e "${GREEN}"
    echo "🎉 ALL TASK 7.2 TESTS PASSED! 🎉"
    echo ""
    echo "✅ Exception safety mechanisms implemented correctly"
    echo "✅ RAII resource management working properly"
    echo "✅ Memory leak detection and prevention active"
    echo "✅ Resource cleanup under stress conditions verified"
    echo "✅ All previous functionality preserved (no regressions)"
    echo ""
    echo "Task 7.2 - Exception Safety and Resource Management - COMPLETED"
    echo -e "${NC}"
    exit 0
else
    echo -e "${RED}"
    echo "❌ TASK 7.2 TESTS FAILED! ❌"
    echo ""
    echo "Some exception safety or resource management tests failed."
    echo "Please review the failed tests and fix any issues before"
    echo "marking task 7.2 as completed."
    echo -e "${NC}"
    exit 1
fi