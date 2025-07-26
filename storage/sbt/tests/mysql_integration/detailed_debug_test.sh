#!/bin/bash

# Detailed debug test for SBT record insertion
set -e

MYSQL_CLIENT="../../../../build/runtime_output_directory/mysql"
MYSQL_HOST="127.0.0.1"
MYSQL_PORT="3306"
MYSQL_USER="root"
MYSQL_PASSWORD=""
MYSQL_DATABASE="test"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

execute_mysql() {
    local sql="$1"
    local description="$2"
    
    print_info "Executing: $description"
    print_info "SQL: $sql"
    
    "$MYSQL_CLIENT" -h"$MYSQL_HOST" -P"$MYSQL_PORT" -u"$MYSQL_USER" \
                    ${MYSQL_PASSWORD:+-p"$MYSQL_PASSWORD"} \
                    -D"$MYSQL_DATABASE" \
                    --protocol=tcp \
                    -e "SET SESSION binlog_format = STATEMENT; $sql" 2>/dev/null
}

execute_mysql_with_output() {
    local sql="$1"
    local description="$2"
    
    print_info "Executing: $description"
    print_info "SQL: $sql"
    
    "$MYSQL_CLIENT" -h"$MYSQL_HOST" -P"$MYSQL_PORT" -u"$MYSQL_USER" \
                    ${MYSQL_PASSWORD:+-p"$MYSQL_PASSWORD"} \
                    -D"$MYSQL_DATABASE" \
                    --protocol=tcp \
                    -e "SET SESSION binlog_format = STATEMENT; $sql" 2>/dev/null
}

main() {
    echo "=========================================="
    echo "SBT Detailed Debug Test"
    echo "=========================================="
    
    # Clear previous error log entries
    print_info "Clearing previous log entries..."
    
    # Step 1: Drop and create table
    print_info "Step 1: Creating test table"
    execute_mysql "DROP TABLE IF EXISTS detailed_debug_test;" "Drop table if exists"
    execute_mysql "CREATE TABLE detailed_debug_test (id INT, name VARCHAR(50)) ENGINE=SBT;" "Create test table"
    
    # Step 2: Check SBT engine status
    print_info "Step 2: Checking SBT engine status"
    local engine_status
    engine_status=$(execute_mysql_with_output "SHOW ENGINES;" "Show all engines")
    echo "$engine_status" | grep -i sbt || echo "SBT engine not found in SHOW ENGINES"
    
    # Step 3: Check table file exists
    print_info "Step 3: Checking if table file exists"
    ls -la ../../../../build/mysql-data/test/detailed_debug_test.sbt 2>/dev/null || echo "Table file not found"
    
    # Step 4: Insert a record with detailed logging
    print_info "Step 4: Inserting a record (check error log for details)"
    execute_mysql "INSERT INTO detailed_debug_test (id, name) VALUES (1, 'Test Record');" "Insert test record"
    print_success "Insert command completed"
    
    # Step 5: Check error log for insert details
    print_info "Step 5: Checking recent error log entries"
    tail -n 20 ../../../../build/mysql-data/mysql-error.log | grep -E "(SBT|ERROR)" || echo "No recent SBT log entries"
    
    # Step 6: Try to count records immediately
    print_info "Step 6: Counting records immediately after insert"
    local count_result
    count_result=$(execute_mysql_with_output "SELECT COUNT(*) FROM detailed_debug_test;" "Count records immediately")
    echo "Count result: $count_result"
    
    # Step 7: Try to select records immediately
    print_info "Step 7: Selecting records immediately after insert"
    local select_result
    select_result=$(execute_mysql_with_output "SELECT * FROM detailed_debug_test;" "Select all records")
    echo "Select result: $select_result"
    
    # Step 8: Check table file size
    print_info "Step 8: Checking table file size after insert"
    ls -la ../../../../build/mysql-data/test/detailed_debug_test.sbt 2>/dev/null || echo "Table file not found"
    
    # Step 9: Force table flush
    print_info "Step 9: Forcing table flush"
    execute_mysql "FLUSH TABLES detailed_debug_test;" "Flush table"
    
    # Step 10: Check count after flush
    print_info "Step 10: Counting records after flush"
    count_result=$(execute_mysql_with_output "SELECT COUNT(*) FROM detailed_debug_test;" "Count records after flush")
    echo "Count result after flush: $count_result"
    
    # Step 11: Check error log again
    print_info "Step 11: Checking error log after flush"
    tail -n 30 ../../../../build/mysql-data/mysql-error.log | grep -E "(SBT|ERROR)" || echo "No recent SBT log entries"
    
    # Cleanup
    execute_mysql "DROP TABLE detailed_debug_test;" "Drop test table"
    
    print_info "Detailed debug test completed"
}

main
