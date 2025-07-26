#!/bin/bash

# Debug test for SBT record insertion
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
    echo "SBT Debug Insert Test"
    echo "=========================================="
    
    # Step 1: Drop and create table
    print_info "Step 1: Creating test table"
    execute_mysql "DROP TABLE IF EXISTS debug_test;" "Drop table if exists"
    execute_mysql "CREATE TABLE debug_test (id INT, name VARCHAR(50)) ENGINE=SBT;" "Create test table"
    
    # Step 2: Check table exists
    print_info "Step 2: Verifying table creation"
    local table_info
    table_info=$(execute_mysql_with_output "SHOW CREATE TABLE debug_test;" "Show table structure")
    echo "$table_info"
    
    # Step 3: Insert a record
    print_info "Step 3: Inserting a record"
    execute_mysql "INSERT INTO debug_test (id, name) VALUES (1, 'Test Record');" "Insert test record"
    print_success "Insert command completed"
    
    # Step 4: Immediately check count (before any close)
    print_info "Step 4: Checking record count immediately after insert"
    local count_result
    count_result=$(execute_mysql_with_output "SELECT COUNT(*) FROM debug_test;" "Count records immediately")
    echo "Count result: $count_result"
    
    # Step 5: Try to select the data
    print_info "Step 5: Selecting data immediately after insert"
    local select_result
    select_result=$(execute_mysql_with_output "SELECT * FROM debug_test;" "Select all records")
    echo "Select result: $select_result"
    
    # Step 6: Force table close and reopen by connecting again
    print_info "Step 6: Reconnecting to force table close/reopen"
    sleep 1
    
    # Step 7: Check count after reconnection
    print_info "Step 7: Checking record count after reconnection"
    count_result=$(execute_mysql_with_output "SELECT COUNT(*) FROM debug_test;" "Count records after reconnection")
    echo "Count result after reconnection: $count_result"
    
    # Step 8: Try select again
    print_info "Step 8: Selecting data after reconnection"
    select_result=$(execute_mysql_with_output "SELECT * FROM debug_test;" "Select all records after reconnection")
    echo "Select result after reconnection: $select_result"
    
    # Cleanup
    execute_mysql "DROP TABLE debug_test;" "Drop test table"
    
    print_info "Debug test completed"
}

main
