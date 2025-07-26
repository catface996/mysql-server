#!/bin/bash

# Integration test for Task 8.2.1 - MySQL Storage Engine Plugin Integration and Activation
# This test verifies that the SBT storage engine plugin can be properly loaded
# into MySQL and is activated correctly.

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test configuration
MYSQL_HOST="127.0.0.1"
MYSQL_PORT="3306"
MYSQL_USER="root"
MYSQL_PROTOCOL="tcp"

# Get the absolute path to build directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$(cd "$SCRIPT_DIR/../../../../build" && pwd)"
MYSQLD_PATH="$BUILD_DIR/runtime_output_directory/mysqld"
MYSQL_CLIENT_PATH="$BUILD_DIR/runtime_output_directory/mysql"
CONFIG_FILE="$BUILD_DIR/my.cnf"
DATA_DIR="$BUILD_DIR/mysql-data"
PID_FILE="$BUILD_DIR/mysql.pid"
ERROR_LOG="$BUILD_DIR/mysql-error.log"

# Test results tracking
TOTAL_TESTS=0
PASSED_TESTS=0

# Function to print test results
print_test_result() {
    local test_name="$1"
    local result="$2"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    if [ "$result" = "PASS" ]; then
        echo -e "${GREEN}✅ PASSED: $test_name${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}❌ FAILED: $test_name${NC}"
    fi
}

# Function to start MySQL server
start_mysql() {
    echo -e "${YELLOW}Starting MySQL server...${NC}"
    
    # Stop any existing MySQL processes first
    stop_mysql
    
    # Clean up any stale files
    rm -f "$PID_FILE" "$ERROR_LOG"
    
    # Ensure data directory exists
    if [ ! -d "$DATA_DIR" ]; then
        echo "Data directory $DATA_DIR does not exist. Please initialize MySQL first."
        return 1
    fi
    
    # Start MySQL server in background
    cd "$BUILD_DIR"
    echo "Starting MySQL with command: $MYSQLD_PATH --defaults-file=$CONFIG_FILE"
    
    # Start MySQL and capture PID
    "$MYSQLD_PATH" --defaults-file="$CONFIG_FILE" &
    local mysql_pid=$!
    echo $mysql_pid > "$PID_FILE"
    
    # Wait for MySQL to start
    local max_attempts=30
    local attempt=0
    
    echo "Waiting for MySQL to start (PID: $mysql_pid)..."
    while [ $attempt -lt $max_attempts ]; do
        # Check if process is still running
        if ! kill -0 "$mysql_pid" 2>/dev/null; then
            echo "MySQL process died unexpectedly"
            if [ -f "$ERROR_LOG" ]; then
                echo "Error log contents:"
                cat "$ERROR_LOG"
            fi
            return 1
        fi
        
        # Check if MySQL is listening on the port (try multiple methods)
        if netstat -an 2>/dev/null | grep -q ":$MYSQL_PORT.*LISTEN" || \
           lsof -i :$MYSQL_PORT 2>/dev/null | grep -q LISTEN || \
           "$MYSQL_CLIENT_PATH" -h "$MYSQL_HOST" -P "$MYSQL_PORT" -u "$MYSQL_USER" --protocol="$MYSQL_PROTOCOL" -e "SELECT 1;" >/dev/null 2>&1; then
            echo "MySQL server started successfully (PID: $mysql_pid)"
            return 0
        fi
        
        sleep 1
        attempt=$((attempt + 1))
        echo -n "."
    done
    
    echo ""
    echo "Failed to start MySQL server within $max_attempts seconds"
    if [ -f "$ERROR_LOG" ]; then
        echo "Error log contents:"
        cat "$ERROR_LOG"
    fi
    return 1
}

# Function to stop MySQL server
stop_mysql() {
    echo -e "${YELLOW}Stopping MySQL server...${NC}"
    
    # Kill by PID file first
    if [ -f "$PID_FILE" ]; then
        local pid=$(cat "$PID_FILE")
        if kill -0 "$pid" 2>/dev/null; then
            echo "Stopping MySQL process $pid"
            kill "$pid" 2>/dev/null || true
            
            # Wait for process to stop
            local max_attempts=10
            local attempt=0
            while [ $attempt -lt $max_attempts ] && kill -0 "$pid" 2>/dev/null; do
                sleep 1
                attempt=$((attempt + 1))
            done
            
            if kill -0 "$pid" 2>/dev/null; then
                echo "Force killing MySQL process $pid"
                kill -9 "$pid" 2>/dev/null || true
            fi
        fi
        rm -f "$PID_FILE"
    fi
    
    # Also kill any remaining mysqld processes that might match our config
    pkill -f "mysqld.*my.cnf" 2>/dev/null || true
    pkill -f "mysqld.*$BUILD_DIR" 2>/dev/null || true
    
    # Wait a moment for cleanup
    sleep 2
    
    echo "MySQL server stopped"
}

# Function to execute SQL command
execute_sql() {
    local sql="$1"
    local output_file=$(mktemp)
    local max_attempts=5
    local attempt=0
    
    cd "$BUILD_DIR"
    
    # Retry connection a few times in case MySQL is still starting
    while [ $attempt -lt $max_attempts ]; do
        if "$MYSQL_CLIENT_PATH" -h "$MYSQL_HOST" -P "$MYSQL_PORT" -u "$MYSQL_USER" --protocol="$MYSQL_PROTOCOL" -e "$sql" > "$output_file" 2>&1; then
            cat "$output_file"
            rm -f "$output_file"
            return 0
        else
            attempt=$((attempt + 1))
            if [ $attempt -lt $max_attempts ]; then
                echo "Connection attempt $attempt failed, retrying..." >&2
                sleep 2
            fi
        fi
    done
    
    echo "Failed to execute SQL after $max_attempts attempts" >&2
    cat "$output_file" >&2
    rm -f "$output_file"
    return 1
}

# Test functions
test_mysql_startup() {
    echo -e "\n${YELLOW}=== Test: MySQL Startup with SBT Plugin ===${NC}"
    
    if start_mysql; then
        print_test_result "MySQL Startup with SBT Plugin" "PASS"
        return 0
    else
        print_test_result "MySQL Startup with SBT Plugin" "FAIL"
        return 1
    fi
}

test_show_engines() {
    echo -e "\n${YELLOW}=== Test: SHOW ENGINES Command ===${NC}"
    
    local output=$(execute_sql "SHOW ENGINES;" 2>/dev/null)
    
    if echo "$output" | grep -q "SBT.*YES"; then
        print_test_result "SBT Engine in SHOW ENGINES" "PASS"
        return 0
    else
        echo "Expected to find 'SBT' with 'YES' status in SHOW ENGINES output"
        echo "Actual output:"
        echo "$output"
        print_test_result "SBT Engine in SHOW ENGINES" "FAIL"
        return 1
    fi
}

test_engine_description() {
    echo -e "\n${YELLOW}=== Test: SBT Engine Description ===${NC}"
    
    local output=$(execute_sql "SHOW ENGINES;" 2>/dev/null)
    
    if echo "$output" | grep -q "Size Balanced Tree Storage Engine"; then
        print_test_result "SBT Engine Description" "PASS"
        return 0
    else
        echo "Expected to find 'Size Balanced Tree Storage Engine' in SHOW ENGINES output"
        echo "Actual output:"
        echo "$output"
        print_test_result "SBT Engine Description" "FAIL"
        return 1
    fi
}

test_status_variables() {
    echo -e "\n${YELLOW}=== Test: SBT Status Variables ===${NC}"
    
    local output=$(execute_sql "SHOW STATUS LIKE 'sbt%';" 2>/dev/null)
    
    # Check for expected status variables
    local expected_vars=("sbt_tables_created" "sbt_tables_opened" "sbt_rows_inserted" "sbt_rows_updated" "sbt_rows_deleted" "sbt_rows_read")
    local all_found=true
    
    for var in "${expected_vars[@]}"; do
        if ! echo "$output" | grep -q "$var"; then
            echo "Status variable '$var' not found"
            all_found=false
        fi
    done
    
    if [ "$all_found" = true ]; then
        print_test_result "SBT Status Variables" "PASS"
        return 0
    else
        echo "Some status variables missing. Actual output:"
        echo "$output"
        print_test_result "SBT Status Variables" "FAIL"
        return 1
    fi
}

test_plugin_info() {
    echo -e "\n${YELLOW}=== Test: Plugin Information ===${NC}"
    
    local output=$(execute_sql "SHOW PLUGINS;" 2>/dev/null)
    
    if echo "$output" | grep -q "SBT.*ACTIVE.*STORAGE ENGINE"; then
        print_test_result "SBT Plugin Information" "PASS"
        return 0
    else
        echo "Expected to find 'SBT' with 'ACTIVE' and 'STORAGE ENGINE' in SHOW PLUGINS output"
        echo "Actual output (filtered for SBT):"
        echo "$output" | grep -i sbt || echo "No SBT entries found"
        print_test_result "SBT Plugin Information" "FAIL"
        return 1
    fi
}

test_handlerton_registration() {
    echo -e "\n${YELLOW}=== Test: Handlerton Structure Registration ===${NC}"
    
    local output=$(execute_sql "SELECT ENGINE, SUPPORT, COMMENT FROM INFORMATION_SCHEMA.ENGINES WHERE ENGINE='SBT';" 2>/dev/null)
    
    if echo "$output" | grep -q "SBT.*YES"; then
        print_test_result "Handlerton Registration" "PASS"
        return 0
    else
        echo "Expected to find SBT engine with YES support in INFORMATION_SCHEMA.ENGINES"
        echo "Actual output:"
        echo "$output"
        print_test_result "Handlerton Registration" "FAIL"
        return 1
    fi
}

# Cleanup function
cleanup() {
    echo -e "\n${YELLOW}Cleaning up...${NC}"
    stop_mysql
}

# Set up cleanup trap
trap cleanup EXIT

# Main test execution
main() {
    echo "=== SBT Storage Engine Integration Test - Task 8.2.1 ==="
    echo "Testing MySQL storage engine plugin integration and activation"
    echo "Build directory: $BUILD_DIR"
    echo "MySQL configuration: $CONFIG_FILE"
    
    # Check if required files exist
    if [ ! -f "$MYSQLD_PATH" ]; then
        echo -e "${RED}Error: MySQL server not found at $MYSQLD_PATH${NC}"
        echo "Please ensure MySQL is compiled first"
        exit 1
    fi
    
    if [ ! -f "$MYSQL_CLIENT_PATH" ]; then
        echo -e "${RED}Error: MySQL client not found at $MYSQL_CLIENT_PATH${NC}"
        echo "Please ensure MySQL client is compiled first"
        exit 1
    fi
    
    if [ ! -f "$CONFIG_FILE" ]; then
        echo -e "${RED}Error: MySQL configuration file not found at $CONFIG_FILE${NC}"
        echo "Please ensure MySQL configuration is set up"
        exit 1
    fi
    
    # Stop any existing MySQL processes
    stop_mysql
    
    # Run all tests
    test_mysql_startup || exit 1
    test_show_engines
    test_engine_description
    test_status_variables
    test_plugin_info
    test_handlerton_registration
    
    # Print final results
    echo -e "\n${YELLOW}=== Test Results ===${NC}"
    echo "Passed: $PASSED_TESTS/$TOTAL_TESTS"
    
    if [ $PASSED_TESTS -eq $TOTAL_TESTS ]; then
        echo -e "${GREEN}🎉 ALL TESTS PASSED! 🎉${NC}"
        echo -e "${GREEN}Task 8.2.1 - MySQL Storage Engine Plugin Integration and Activation: COMPLETED${NC}"
        exit 0
    else
        echo -e "${RED}❌ SOME TESTS FAILED${NC}"
        exit 1
    fi
}

# Run main function
main "$@"