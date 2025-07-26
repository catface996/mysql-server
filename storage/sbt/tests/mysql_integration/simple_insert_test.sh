#!/bin/bash

# Simple INSERT test for SBT storage engine
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

main() {
    echo "=========================================="
    echo "SBT Simple INSERT Test"
    echo "=========================================="
    
    # Step 1: Create table
    print_info "Step 1: Creating test table"
    execute_mysql "DROP TABLE IF EXISTS simple_insert_test;" "Drop table if exists"
    execute_mysql "CREATE TABLE simple_insert_test (id INT, name VARCHAR(50)) ENGINE=SBT;" "Create test table"
    
    # Step 2: Insert a record
    print_info "Step 2: Inserting a record"
    execute_mysql "INSERT INTO simple_insert_test (id, name) VALUES (1, 'Test Record');" "Insert test record"
    print_success "Insert command completed"
    
    # Step 3: Check error log immediately
    print_info "Step 3: Checking error log for debug messages"
    tail -n 30 ../../../../build/mysql-data/mysql-error.log | grep -E "(WRITE_ROW|OPEN|EXTERNAL_LOCK|SBT)" | tail -10
    
    # Cleanup
    execute_mysql "DROP TABLE simple_insert_test;" "Drop test table"
    
    print_info "Simple INSERT test completed"
}

main
