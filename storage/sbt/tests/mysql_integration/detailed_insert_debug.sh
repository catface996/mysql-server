#!/bin/bash

# Detailed INSERT debug test for SBT storage engine
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
    tail -n 20 ../../../../build/mysql-data/mysql-error.log | grep -E "(SBT|WRITE_ROW|INSERT|RND_)" | tail -10 || echo "No relevant messages found"
}

main() {
    echo "=========================================="
    echo "SBT Detailed INSERT Debug Test"
    echo "=========================================="
    
    # Step 1: Check SBT engine status
    print_info "Step 1: Checking SBT engine status"
    execute_mysql_with_output "SHOW ENGINES;" "Show all engines"
    execute_mysql_with_output "SHOW ENGINES;" "Show all engines" | grep -i sbt || print_error "SBT engine not found"
    
    # Step 2: Create table
    print_info "Step 2: Creating debug test table"
    execute_mysql "DROP TABLE IF EXISTS detailed_insert_debug;" "Drop table if exists"
    execute_mysql "CREATE TABLE detailed_insert_debug (id INT, name VARCHAR(50)) ENGINE=SBT;" "Create test table"
    
    check_error_log
    
    # Step 3: Check table info
    print_info "Step 3: Checking table information"
    execute_mysql_with_output "SHOW CREATE TABLE detailed_insert_debug;" "Show table structure"
    execute_mysql_with_output "SHOW TABLE STATUS LIKE 'detailed_insert_debug';" "Show table status"
    
    # Step 4: Insert with explicit transaction
    print_info "Step 4: Inserting record with explicit transaction"
    execute_mysql "START TRANSACTION;" "Start transaction"
    execute_mysql "INSERT INTO detailed_insert_debug (id, name) VALUES (1, 'Test Record 1');" "Insert first record"
    execute_mysql "COMMIT;" "Commit transaction"
    
    check_error_log
    
    # Step 5: Try to select the record
    print_info "Step 5: Selecting the record"
    execute_mysql_with_output "SELECT * FROM detailed_insert_debug;" "Select all records"
    execute_mysql_with_output "SELECT COUNT(*) FROM detailed_insert_debug;" "Count records"
    
    check_error_log
    
    # Step 6: Insert without transaction
    print_info "Step 6: Inserting record without explicit transaction"
    execute_mysql "INSERT INTO detailed_insert_debug (id, name) VALUES (2, 'Test Record 2');" "Insert second record"
    
    check_error_log
    
    # Step 7: Select again
    print_info "Step 7: Selecting all records again"
    execute_mysql_with_output "SELECT * FROM detailed_insert_debug;" "Select all records"
    execute_mysql_with_output "SELECT COUNT(*) FROM detailed_insert_debug;" "Count records"
    
    check_error_log
    
    # Step 8: Check table file
    print_info "Step 8: Checking table file"
    ls -la ../../../../build/test/detailed_insert_debug.sbt 2>/dev/null || print_warning "Table file not found"
    
    # Cleanup
    execute_mysql "DROP TABLE detailed_insert_debug;" "Drop test table"
    
    print_info "Detailed INSERT debug test completed"
}

main
