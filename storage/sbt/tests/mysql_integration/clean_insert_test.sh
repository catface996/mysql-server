#!/bin/bash

# Clean INSERT test for SBT storage engine without binlog settings
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

execute_mysql_clean() {
    local sql="$1"
    local description="$2"
    
    print_info "Executing: $description"
    print_info "SQL: $sql"
    
    "$MYSQL_CLIENT" -h"$MYSQL_HOST" -P"$MYSQL_PORT" -u"$MYSQL_USER" \
                    ${MYSQL_PASSWORD:+-p"$MYSQL_PASSWORD"} \
                    -D"$MYSQL_DATABASE" \
                    --protocol=tcp \
                    -e "$sql" 2>/dev/null
}

execute_mysql_with_output_clean() {
    local sql="$1"
    local description="$2"
    
    print_info "Executing: $description"
    print_info "SQL: $sql"
    
    local output
    output=$("$MYSQL_CLIENT" -h"$MYSQL_HOST" -P"$MYSQL_PORT" -u"$MYSQL_USER" \
                    ${MYSQL_PASSWORD:+-p"$MYSQL_PASSWORD"} \
                    -D"$MYSQL_DATABASE" \
                    --protocol=tcp \
                    -e "$sql" 2>/dev/null)
    
    echo "Output: $output"
    return 0
}

main() {
    echo "=========================================="
    echo "SBT Clean INSERT Test (No binlog settings)"
    echo "=========================================="
    
    # Step 1: Create table
    print_info "Step 1: Creating test table"
    execute_mysql_clean "DROP TABLE IF EXISTS clean_insert_test;" "Drop table if exists"
    execute_mysql_clean "CREATE TABLE clean_insert_test (id INT, name VARCHAR(50)) ENGINE=SBT;" "Create test table"
    
    # Step 2: Insert a record
    print_info "Step 2: Inserting a record"
    execute_mysql_clean "INSERT INTO clean_insert_test (id, name) VALUES (1, 'Test Record');" "Insert test record"
    print_success "Insert command completed"
    
    # Step 3: Select the record
    print_info "Step 3: Selecting the record"
    execute_mysql_with_output_clean "SELECT * FROM clean_insert_test;" "Select all records"
    
    # Step 4: Count records
    print_info "Step 4: Counting records"
    execute_mysql_with_output_clean "SELECT COUNT(*) FROM clean_insert_test;" "Count records"
    
    # Step 5: Insert another record
    print_info "Step 5: Inserting another record"
    execute_mysql_clean "INSERT INTO clean_insert_test (id, name) VALUES (2, 'Second Record');" "Insert second record"
    
    # Step 6: Select all records
    print_info "Step 6: Selecting all records"
    execute_mysql_with_output_clean "SELECT * FROM clean_insert_test;" "Select all records"
    
    # Cleanup
    execute_mysql_clean "DROP TABLE clean_insert_test;" "Drop test table"
    
    print_info "Clean INSERT test completed"
}

main
