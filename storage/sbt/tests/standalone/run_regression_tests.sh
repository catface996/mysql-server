#!/bin/bash

# SBT Storage Engine Regression Test Runner
# This script runs all regression tests for completed tasks

set -e  # Exit on any error

echo "=== SBT Storage Engine Regression Test Suite ==="
echo "Testing all completed tasks for regressions..."
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test results tracking
TOTAL_TESTS=0
PASSED_TESTS=0

# Function to run a test and report results
run_test() {
    local test_name="$1"
    local test_command="$2"
    
    echo -e "${YELLOW}Running: $test_name${NC}"
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    if eval "$test_command"; then
        echo -e "${GREEN}✓ PASSED: $test_name${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}✗ FAILED: $test_name${NC}"
        echo -e "${RED}❌ REGRESSION DETECTED in $test_name${NC}"
        return 1
    fi
    echo ""
}

# Clean up previous builds
echo "Cleaning up previous builds..."
make clean 2>/dev/null || true
rm -f test_insertion_standalone test_deletion_standalone test_search_traversal_standalone regression_test_all

# Run individual task tests
run_test "Task 2.1 & 2.2: Data Structures and Insertion" "make test_insertion_standalone && ./test_insertion_standalone"
run_test "Task 2.3: Deletion Operations" "make test_deletion_standalone && ./test_deletion_standalone"
run_test "Task 2.4: Search and Traversal" "make test_search_traversal_standalone && ./test_search_traversal_standalone"
run_test "Task 3.1: File Format" "./verify_file_format.sh"

# Run serialization test if it exists
if [ -f "verify_serialization.sh" ]; then
    run_test "Task 3.2: Serialization" "./verify_serialization.sh"
fi

# Run shared resource management tests
run_test "Task 4.1: SBT_share Class Implementation" "make test_sbt_share_standalone && ./test_sbt_share_standalone"
run_test "Task 4.2: Shared Resource Management" "make test_task_4_2_verification && ./test_task_4_2_verification"

# Run MySQL Handler interface tests
run_test "Task 5.1: ha_sbt Class Basic Structure" "make test_task_5_1_verification && ./test_task_5_1_verification"
run_test "Task 5.4: Record Insertion Operations" "make test_task_5_4_verification && ./test_task_5_4_verification"
run_test "Task 5.6: Record Deletion Operations" "make test_task_5_6_verification && ./test_task_5_6_verification"

# Run comprehensive regression test
run_test "Comprehensive Integration Test" "g++ -std=c++17 -Wall -Wextra -O2 -o regression_test_all regression_test_all.cc && ./regression_test_all"

# Final results
echo "=== Regression Test Results ==="
echo "Total tests: $TOTAL_TESTS"
echo "Passed: $PASSED_TESTS"
echo "Failed: $((TOTAL_TESTS - PASSED_TESTS))"

if [ $PASSED_TESTS -eq $TOTAL_TESTS ]; then
    echo -e "${GREEN}"
    echo "🎉 ALL REGRESSION TESTS PASSED! 🎉"
    echo "No regressions detected in SBT storage engine functionality."
    echo "All previously completed tasks continue to work correctly."
    echo -e "${NC}"
    exit 0
else
    echo -e "${RED}"
    echo "❌ REGRESSION DETECTED! ❌"
    echo "Some previously working functionality has been broken."
    echo "Please review the failed tests and fix any regressions before proceeding."
    echo -e "${NC}"
    exit 1
fi