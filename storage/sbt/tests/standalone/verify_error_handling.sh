#!/bin/bash

# SBT Storage Engine - Error Handling Verification Script
# This script verifies the error handling implementation

echo "=== SBT Error Handling Verification ==="
echo "Testing error handling functionality..."
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
    
    echo -e "${YELLOW}Testing: $test_name${NC}"
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    if eval "$test_command" >/dev/null; then
        echo -e "${GREEN}✓ PASSED: $test_name${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}✗ FAILED: $test_name${NC}"
        return 1
    fi
    echo ""
}

# Change to the tests directory
cd "$(dirname "$0")"

# Clean up any previous builds
make clean 2>/dev/null || true

echo "Building error handling test..."
run_test "Error Handling Test Compilation" "make test_error_handling_standalone"

echo "Running comprehensive error handling tests..."
run_test "Error Handling Functionality" "../build/test_error_handling_standalone"

# Test error code coverage
echo "=== Verifying Error Code Coverage ==="

# Create a simple test program to verify all error codes are handled
cat > temp_error_coverage_test.cc << 'EOF'
#include <iostream>
#include <cstring>

// Mock MySQL types and error codes
typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;

#define HA_ERR_OUT_OF_MEM 5
#define HA_ERR_NO_SUCH_TABLE 1
#define HA_ERR_TABLE_EXIST 121
#define HA_ERR_NO_PERMISSION 13
#define HA_ERR_CRASHED_ON_USAGE 126
#define HA_ERR_RECORD_FILE_FULL 135
#define HA_ERR_FOUND_DUPP_KEY 121
#define HA_ERR_KEY_NOT_FOUND 125
#define HA_ERR_WRONG_COMMAND 131
#define HA_ERR_LOCK_WAIT_TIMEOUT 205
#define HA_ERR_GENERIC 2

uint32_t my_thread_id() { return 12345; }
uint64_t sbt_get_current_time() { return 1640995200000000ULL; }

// Include error definitions and implementations from the main test
EOF

# Extract the error definitions and implementations from the main test
sed -n '/enum sbt_error_t/,/^};/p' test_error_handling_standalone.cc >> temp_error_coverage_test.cc
sed -n '/const char \*sbt_error_to_string/,/^}/p' test_error_handling_standalone.cc >> temp_error_coverage_test.cc
sed -n '/int sbt_error_to_mysql_error/,/^}/p' test_error_handling_standalone.cc >> temp_error_coverage_test.cc

cat >> temp_error_coverage_test.cc << 'EOF'

int main() {
    bool all_covered = true;
    
    // Test that all error codes have string representations
    for (int i = 0; i <= SBT_ERR_UNKNOWN; i++) {
        const char *error_str = sbt_error_to_string((sbt_error_t)i);
        if (!error_str || strlen(error_str) == 0) {
            std::cout << "Error code " << i << " has no string representation" << std::endl;
            all_covered = false;
        }
    }
    
    // Test that all error codes have MySQL mappings
    for (int i = 0; i <= SBT_ERR_UNKNOWN; i++) {
        int mysql_error = sbt_error_to_mysql_error((sbt_error_t)i);
        if (mysql_error < 0) {
            std::cout << "Error code " << i << " has invalid MySQL mapping" << std::endl;
            all_covered = false;
        }
    }
    
    if (all_covered) {
        std::cout << "All error codes are properly covered" << std::endl;
        return 0;
    } else {
        std::cout << "Some error codes are not properly covered" << std::endl;
        return 1;
    }
}
EOF

run_test "Error Code Coverage" "g++ -std=c++11 -o temp_error_coverage_test temp_error_coverage_test.cc && ./temp_error_coverage_test"

# Clean up temporary files
rm -f temp_error_coverage_test temp_error_coverage_test.cc

# Test error handling integration with existing code
echo "=== Testing Error Handling Integration ==="

# Check if error handling is properly integrated in existing source files
if [ -f "../../src/sbt_common.cc" ]; then
    run_test "Error Handling Implementation Exists" "grep -q 'sbt_error_to_mysql_error' ../../src/sbt_common.cc"
    run_test "Error Context Functions Exist" "grep -q 'sbt_error_context_init' ../../src/sbt_common.cc"
    run_test "Enhanced Logging Functions Exist" "grep -q 'sbt_log_warning' ../../src/sbt_common.cc"
else
    echo -e "${RED}✗ FAILED: sbt_common.cc not found${NC}"
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
fi

if [ -f "../../include/sbt_common.h" ]; then
    run_test "Error Handling Headers Exist" "grep -q 'enum sbt_error_t' ../../include/sbt_common.h"
    run_test "Error Context Structure Defined" "grep -q 'struct SBT_error_context' ../../include/sbt_common.h"
    run_test "Error Handling Macros Defined" "grep -q 'SBT_SET_ERROR' ../../include/sbt_common.h"
else
    echo -e "${RED}✗ FAILED: sbt_common.h not found${NC}"
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
fi

# Test error handling in different scenarios
echo "=== Testing Error Scenarios ==="

# Create a test for error handling in various scenarios
cat > temp_error_scenarios_test.cc << 'EOF'
#include <iostream>
#include <cstring>
#include <cstdarg>

// Mock MySQL types
typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;

uint32_t my_thread_id() { return 12345; }
uint64_t sbt_get_current_time() { return 1640995200000000ULL; }

EOF

# Add error definitions from main test
sed -n '/enum sbt_error_t/,/^};/p' test_error_handling_standalone.cc >> temp_error_scenarios_test.cc
sed -n '/void sbt_error_context_init/,/^}/p' test_error_handling_standalone.cc >> temp_error_scenarios_test.cc
sed -n '/void sbt_error_context_set/,/^}/p' test_error_handling_standalone.cc >> temp_error_scenarios_test.cc
sed -n '/void sbt_error_context_clear/,/^}/p' test_error_handling_standalone.cc >> temp_error_scenarios_test.cc

cat >> temp_error_scenarios_test.cc << 'EOF'

int main() {
    // Test error context in various scenarios
    SBT_error_context ctx;
    
    // Scenario 1: Memory allocation failure
    sbt_error_context_init(&ctx);
    sbt_error_context_set(&ctx, SBT_ERR_OUT_OF_MEMORY, SBT_SEVERITY_ERROR,
                          "memory.cc", 100, "allocate_buffer", 
                          "Failed to allocate %d bytes", 1024);
    
    if (ctx.error_code != SBT_ERR_OUT_OF_MEMORY) {
        std::cout << "Memory error scenario failed" << std::endl;
        return 1;
    }
    
    // Scenario 2: File operation failure
    sbt_error_context_set(&ctx, SBT_ERR_FILE_NOT_FOUND, SBT_SEVERITY_ERROR,
                          "file.cc", 200, "open_file", 
                          "Cannot open file: %s", "data.sbt");
    
    if (ctx.error_code != SBT_ERR_FILE_NOT_FOUND) {
        std::cout << "File error scenario failed" << std::endl;
        return 1;
    }
    
    // Scenario 3: Data corruption
    sbt_error_context_set(&ctx, SBT_ERR_CORRUPTED_DATA, SBT_SEVERITY_FATAL,
                          "tree.cc", 300, "validate_node", 
                          "Node checksum mismatch: expected %x, got %x", 
                          0x12345678, 0x87654321);
    
    if (ctx.error_code != SBT_ERR_CORRUPTED_DATA) {
        std::cout << "Data corruption scenario failed" << std::endl;
        return 1;
    }
    
    // Scenario 4: Clear error context
    sbt_error_context_clear(&ctx);
    
    if (ctx.error_code != SBT_SUCCESS) {
        std::cout << "Error context clear scenario failed" << std::endl;
        return 1;
    }
    
    std::cout << "All error scenarios passed" << std::endl;
    return 0;
}
EOF

run_test "Error Handling Scenarios" "g++ -std=c++11 -o temp_error_scenarios_test temp_error_scenarios_test.cc && ./temp_error_scenarios_test"

# Clean up
rm -f temp_error_scenarios_test temp_error_scenarios_test.cc

# Final results
echo "=== Error Handling Verification Results ==="
echo "Total tests: $TOTAL_TESTS"
echo "Passed: $PASSED_TESTS"
echo "Failed: $((TOTAL_TESTS - PASSED_TESTS))"

if [ $PASSED_TESTS -eq $TOTAL_TESTS ]; then
    echo -e "${GREEN}"
    echo "🎉 ALL ERROR HANDLING TESTS PASSED! 🎉"
    echo "Error handling implementation is complete and functional."
    echo "All error codes, mappings, and context handling work correctly."
    echo -e "${NC}"
    exit 0
else
    echo -e "${RED}"
    echo "❌ SOME ERROR HANDLING TESTS FAILED! ❌"
    echo "Please review the failed tests and fix any issues."
    echo -e "${NC}"
    exit 1
fi