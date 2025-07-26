#!/bin/bash

# Debug INSERT and SELECT operations for SBT storage engine
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
YELLOW='\033[1;33m'
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

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
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
    
    local output
    output=$("$MYSQL_CLIENT" -h"$MYSQL_HOST" -P"$MYSQL_PORT" -u"$MYSQL_USER" \
                    ${MYSQL_PASSWORD:+-p"$MYSQL_PASSWORD"} \
                    -D"$MYSQL_DATABASE" \
                    --protocol=tcp \
                    -e "SET SESSION binlog_format = STATEMENT; $sql" 2>/dev/null)
    
    echo "Output: $output"
    return 0
}

check_error_log() {
    print_info "Checking error log for recent SBT messages..."
    tail -n 50 ../../../../build/mysql-data/mysql-error.log | grep -E "(SBT|WRITE_ROW|INSERT|SELECT|RND_)" | tail -20 || true
}

main() {
    echo "=========================================="
    echo "SBT Debug INSERT and SELECT Test"
    echo "=========================================="
    
    # Step 1: Create table
    print_info "Step 1: Creating debug test table"
    execute_mysql "DROP TABLE IF EXISTS debug_insert_test;" "Drop table if exists"
    execute_mysql "CREATE TABLE debug_insert_test (id INT, name VARCHAR(50)) ENGINE=SBT;" "Create test table"
    
    check_error_log
    
    # Step 2: Insert a record
    print_info "Step 2: Inserting a record"
    execute_mysql "INSERT INTO debug_insert_test (id, name) VALUES (1, 'Test Record');" "Insert test record"
    print_success "Insert command completed"
    
    check_error_log
    
    # Step 3: Try to select the record
    print_info "Step 3: Selecting the record"
    execute_mysql_with_output "SELECT * FROM debug_insert_test;" "Select all records"
    
    check_error_log
    
    # Step 4: Check record count
    print_info "Step 4: Checking record count"
    execute_mysql_with_output "SELECT COUNT(*) FROM debug_insert_test;" "Count records"
    
    check_error_log
    
    # Step 5: Insert another record
    print_info "Step 5: Inserting another record"
    execute_mysql "INSERT INTO debug_insert_test (id, name) VALUES (2, 'Second Record');" "Insert second record"
    
    check_error_log
    
    # Step 6: Select all records again
    print_info "Step 6: Selecting all records"
    execute_mysql_with_output "SELECT * FROM debug_insert_test;" "Select all records again"
    
    check_error_log
    
    # Cleanup
    execute_mysql "DROP TABLE debug_insert_test;" "Drop test table"
    
    print_info "Debug test completed"
}

main
