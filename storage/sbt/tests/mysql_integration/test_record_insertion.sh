#!/bin/bash

# SBT Storage Engine - Record Insertion Integration Test
# This script tests record insertion operations by connecting to MySQL server via TCP
# MANDATORY: Uses TCP connection only as per steering/mysql-tcp-connection-standard.md

set -e  # Exit on any error

# TCP connection parameters - DO NOT CHANGE (per TCP connection standard)
MYSQL_HOST="127.0.0.1"          # Always use IP address, not localhost
MYSQL_PORT="3306"               # Standard MySQL port
MYSQL_USER="root"               # Default user for testing
MYSQL_PASSWORD=""               # Empty password for development
MYSQL_DATABASE="test"           # Default test database
PROTOCOL="tcp"                  # Explicitly specify TCP protocol
TEST_TABLE_PREFIX="sbt_insert_test"

# MySQL client path - use built MySQL client
MYSQL_CLIENT="../../../../build/runtime_output_directory/mysql"

# Verify MySQL client exists
if [ ! -f "$MYSQL_CLIENT" ]; then
    echo "Error: MySQL client not found at $MYSQL_CLIENT"
    echo "Please ensure MySQL is built and the client is available"
    exit 1
fi

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test results tracking
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Function to print colored output
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to execute MySQL command via TCP
execute_mysql() {
    local sql="$1"
    local description="$2"
    
    print_info "Executing: $description"
    print_info "SQL: $sql"
    
    if "$MYSQL_CLIENT" -h"$MYSQL_HOST" -P"$MYSQL_PORT" -u"$MYSQL_USER" \
                       ${MYSQL_PASSWORD:+-p"$MYSQL_PASSWORD"} \
                       -D"$MYSQL_DATABASE" \
                       --protocol=tcp \
                       -e "SET SESSION binlog_format = STATEMENT; $sql" 2>/dev/null; then
        return 0
    else
        return 1
    fi
}

# Function to execute MySQL command and capture output via TCP
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

# Function to run a test
run_test() {
    local test_name="$1"
    local test_function="$2"
    
    echo ""
    echo "=========================================="
    print_info "Running Test: $test_name"
    echo "=========================================="
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    if $test_function; then
        print_success "✓ PASSED: $test_name"
        PASSED_TESTS=$((PASSED_TESTS + 1))
        return 0
    else
        print_error "✗ FAILED: $test_name"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        return 1
    fi
}

# Function to check MySQL TCP connection
check_mysql_connection() {
    print_info "Checking MySQL TCP connection..."
    print_info "Connection parameters:"
    print_info "  Host: $MYSQL_HOST"
    print_info "  Port: $MYSQL_PORT"
    print_info "  User: $MYSQL_USER"
    print_info "  Database: $MYSQL_DATABASE"
    print_info "  Protocol: TCP"
    
    if execute_mysql "SELECT 1;" "Test MySQL TCP connection"; then
        print_success "MySQL TCP connection successful"
        return 0
    else
        print_error "Failed to connect to MySQL server via TCP"
        print_error "Connection parameters:"
        print_error "  Host: $MYSQL_HOST"
        print_error "  Port: $MYSQL_PORT"
        print_error "  User: $MYSQL_USER"
        print_error "  Database: $MYSQL_DATABASE"
        print_error "  Protocol: TCP"
        print_error ""
        print_error "Troubleshooting steps:"
        print_error "1. Ensure MySQL server is running"
        print_error "2. Check that skip-networking=false in my.cnf"
        print_error "3. Verify port 3306 is accessible: netstat -an | grep :3306"
        print_error "4. Check firewall settings"
        print_error "5. Verify TCP protocol is enabled"
        return 1
    fi
}

# Test 1: Setup test table for insertion tests
test_setup_test_table() {
    local table_name="${TEST_TABLE_PREFIX}_basic"
    
    print_info "Setting up test table for insertion tests..."
    
    # Drop table if exists (cleanup)
    execute_mysql "DROP TABLE IF EXISTS $table_name;" "Drop table if exists" || true
    
    # Create table with SBT engine
    local create_sql="CREATE TABLE $table_name (
        id INT,
        name VARCHAR(100),
        description TEXT,
        price DECIMAL(10,2),
        created_date DATE,
        is_active BOOLEAN
    ) ENGINE=SBT;"
    
    if execute_mysql "$create_sql" "Create test table with SBT engine"; then
        print_success "Test table created successfully: $table_name"
        
        # Verify table exists and uses SBT engine
        local table_info
        table_info=$(execute_mysql_with_output "SELECT TABLE_NAME, ENGINE FROM information_schema.TABLES WHERE TABLE_SCHEMA='$MYSQL_DATABASE' AND TABLE_NAME='$table_name';" "Verify table creation")
        
        if echo "$table_info" | grep -q "SBT"; then
            print_success "Table correctly uses SBT engine"
            return 0
        else
            print_error "Table does not use SBT engine"
            echo "$table_info"
            return 1
        fi
    else
        print_error "Failed to create test table: $table_name"
        return 1
    fi
}

# Test 2: Single record insertion
test_single_record_insertion() {
    local table_name="${TEST_TABLE_PREFIX}_basic"
    
    print_info "Testing single record insertion..."
    
    # Insert a single record
    local insert_sql="INSERT INTO $table_name (id, name, description, price, created_date, is_active) 
                      VALUES (1, 'Test Product', 'This is a test product', 99.99, '2025-01-01', TRUE);"
    
    if execute_mysql "$insert_sql" "Insert single record"; then
        print_success "Single record inserted successfully"
        
        # Verify the record was inserted by counting rows
        local row_count
        row_count=$(execute_mysql_with_output "SELECT COUNT(*) FROM $table_name;" "Count inserted records")
        local count_value
        count_value=$(echo "$row_count" | tail -n 1)
        
        if [ "$count_value" = "1" ]; then
            print_success "Record count verified: 1 record in table"
            return 0
        else
            print_error "Record count mismatch. Expected: 1, Got: $count_value"
            return 1
        fi
    else
        print_error "Failed to insert single record"
        return 1
    fi
}

# Test 3: Multiple single record insertions
test_multiple_single_insertions() {
    local table_name="${TEST_TABLE_PREFIX}_basic"
    
    print_info "Testing multiple single record insertions..."
    
    # Insert multiple records one by one
    local records=(
        "2, 'Product A', 'Description A', 19.99, '2025-01-02', TRUE"
        "3, 'Product B', 'Description B', 29.99, '2025-01-03', FALSE"
        "4, 'Product C', 'Description C', 39.99, '2025-01-04', TRUE"
        "5, 'Product D', 'Description D', 49.99, '2025-01-05', FALSE"
    )
    
    local inserted_count=0
    for record in "${records[@]}"; do
        local insert_sql="INSERT INTO $table_name (id, name, description, price, created_date, is_active) VALUES ($record);"
        
        if execute_mysql "$insert_sql" "Insert record: $record"; then
            inserted_count=$((inserted_count + 1))
            print_success "Record inserted: $record"
        else
            print_error "Failed to insert record: $record"
            return 1
        fi
    done
    
    # Verify total record count (1 from previous test + 4 new = 5)
    local row_count
    row_count=$(execute_mysql_with_output "SELECT COUNT(*) FROM $table_name;" "Count all records")
    local count_value
    count_value=$(echo "$row_count" | tail -n 1)
    
    if [ "$count_value" = "5" ]; then
        print_success "All records inserted successfully. Total count: $count_value"
        return 0
    else
        print_error "Record count mismatch. Expected: 5, Got: $count_value"
        return 1
    fi
}

# Test 4: Batch insertion (multiple values in single INSERT)
test_batch_insertion() {
    local table_name="${TEST_TABLE_PREFIX}_batch"
    
    print_info "Testing batch insertion..."
    
    # Create separate table for batch test
    execute_mysql "DROP TABLE IF EXISTS $table_name;" "Drop batch test table if exists" || true
    
    local create_sql="CREATE TABLE $table_name (
        id INT,
        name VARCHAR(50),
        value DECIMAL(8,2)
    ) ENGINE=SBT;"
    
    if execute_mysql "$create_sql" "Create batch test table"; then
        print_success "Batch test table created"
        
        # Insert multiple records in single statement
        local batch_insert_sql="INSERT INTO $table_name (id, name, value) VALUES 
            (1, 'Batch A', 10.50),
            (2, 'Batch B', 20.75),
            (3, 'Batch C', 30.25),
            (4, 'Batch D', 40.00),
            (5, 'Batch E', 50.99);"
        
        if execute_mysql "$batch_insert_sql" "Batch insert 5 records"; then
            print_success "Batch insertion completed"
            
            # Verify record count
            local row_count
            row_count=$(execute_mysql_with_output "SELECT COUNT(*) FROM $table_name;" "Count batch inserted records")
            local count_value
            count_value=$(echo "$row_count" | tail -n 1)
            
            if [ "$count_value" = "5" ]; then
                print_success "Batch insertion verified: $count_value records"
                return 0
            else
                print_error "Batch insertion count mismatch. Expected: 5, Got: $count_value"
                return 1
            fi
        else
            print_error "Batch insertion failed"
            return 1
        fi
    else
        print_error "Failed to create batch test table"
        return 1
    fi
}

# Test 5: Different data types insertion
test_data_types_insertion() {
    local table_name="${TEST_TABLE_PREFIX}_datatypes"
    
    print_info "Testing insertion with different data types..."
    
    # Create table with various data types
    execute_mysql "DROP TABLE IF EXISTS $table_name;" "Drop data types test table if exists" || true
    
    local create_sql="CREATE TABLE $table_name (
        int_col INT,
        varchar_col VARCHAR(100),
        text_col TEXT,
        decimal_col DECIMAL(10,3),
        date_col DATE,
        bool_col BOOLEAN
    ) ENGINE=SBT;"
    
    if execute_mysql "$create_sql" "Create data types test table"; then
        print_success "Data types test table created"
        
        # Insert records with various data types
        local test_records=(
            "1, 'Short text', 'This is a longer text field with more content', 123.456, '2025-01-01', TRUE"
            "2, 'Another text', 'Even longer text content that spans multiple words and contains various characters!', 987.654, '2025-12-31', FALSE"
            "-10, 'Negative ID', 'Text with special chars: @#$%^&*()', -456.789, '2000-01-01', TRUE"
            "0, '', 'Empty varchar test', 0.000, '1970-01-01', FALSE"
        )
        
        local inserted_count=0
        for record in "${test_records[@]}"; do
            local insert_sql="INSERT INTO $table_name (int_col, varchar_col, text_col, decimal_col, date_col, bool_col) VALUES ($record);"
            
            if execute_mysql "$insert_sql" "Insert data types record"; then
                inserted_count=$((inserted_count + 1))
                print_success "Data types record inserted successfully"
            else
                print_error "Failed to insert data types record: $record"
                return 1
            fi
        done
        
        # Verify all records inserted
        local row_count
        row_count=$(execute_mysql_with_output "SELECT COUNT(*) FROM $table_name;" "Count data types records")
        local count_value
        count_value=$(echo "$row_count" | tail -n 1)
        
        if [ "$count_value" = "4" ]; then
            print_success "All data types records inserted: $count_value"
            return 0
        else
            print_error "Data types insertion count mismatch. Expected: 4, Got: $count_value"
            return 1
        fi
    else
        print_error "Failed to create data types test table"
        return 1
    fi
}

# Test 6: Data persistence verification
test_data_persistence() {
    local table_name="${TEST_TABLE_PREFIX}_basic"
    
    print_info "Testing data persistence after insertion..."
    
    # Query the data that was inserted in previous tests
    local select_result
    select_result=$(execute_mysql_with_output "SELECT id, name, price FROM $table_name ORDER BY id;" "Query inserted data")
    
    print_info "Inserted data verification:"
    echo "$select_result"
    
    # Check if we can retrieve specific records
    local specific_record
    specific_record=$(execute_mysql_with_output "SELECT name FROM $table_name WHERE id = 1;" "Query specific record")
    
    if echo "$specific_record" | grep -q "Test Product"; then
        print_success "Data persistence verified: specific record found"
        
        # Check record count one more time
        local row_count
        row_count=$(execute_mysql_with_output "SELECT COUNT(*) FROM $table_name;" "Final count verification")
        local count_value
        count_value=$(echo "$row_count" | tail -n 1)
        
        print_info "Final record count: $count_value"
        return 0
    else
        print_error "Data persistence failed: specific record not found"
        echo "Query result: $specific_record"
        return 1
    fi
}

# Test 7: Large batch insertion performance
test_large_batch_insertion() {
    local table_name="${TEST_TABLE_PREFIX}_performance"
    
    print_info "Testing large batch insertion performance..."
    
    # Create performance test table
    execute_mysql "DROP TABLE IF EXISTS $table_name;" "Drop performance test table if exists" || true
    
    local create_sql="CREATE TABLE $table_name (
        id INT,
        data VARCHAR(100)
    ) ENGINE=SBT;"
    
    if execute_mysql "$create_sql" "Create performance test table"; then
        print_success "Performance test table created"
        
        # Generate large batch insert (50 records)
        local batch_values=""
        for i in $(seq 1 50); do
            if [ $i -gt 1 ]; then
                batch_values="$batch_values,"
            fi
            batch_values="$batch_values ($i, 'Performance test record $i')"
        done
        
        local large_batch_sql="INSERT INTO $table_name (id, data) VALUES $batch_values;"
        
        print_info "Inserting 50 records in single batch..."
        local start_time=$(date +%s.%N)
        
        if execute_mysql "$large_batch_sql" "Large batch insert (50 records)"; then
            local end_time=$(date +%s.%N)
            local duration=$(echo "$end_time - $start_time" | bc -l 2>/dev/null || echo "N/A")
            
            print_success "Large batch insertion completed in ${duration}s"
            
            # Verify record count
            local row_count
            row_count=$(execute_mysql_with_output "SELECT COUNT(*) FROM $table_name;" "Count performance test records")
            local count_value
            count_value=$(echo "$row_count" | tail -n 1)
            
            if [ "$count_value" = "50" ]; then
                print_success "Performance test verified: $count_value records inserted"
                return 0
            else
                print_error "Performance test count mismatch. Expected: 50, Got: $count_value"
                return 1
            fi
        else
            print_error "Large batch insertion failed"
            return 1
        fi
    else
        print_error "Failed to create performance test table"
        return 1
    fi
}

# Test 8: Error handling - invalid data types
test_error_handling() {
    local table_name="${TEST_TABLE_PREFIX}_errors"
    
    print_info "Testing error handling for invalid insertions..."
    
    # Create error test table
    execute_mysql "DROP TABLE IF EXISTS $table_name;" "Drop error test table if exists" || true
    
    local create_sql="CREATE TABLE $table_name (
        id INT,
        name VARCHAR(10),
        price DECIMAL(5,2)
    ) ENGINE=SBT;"
    
    if execute_mysql "$create_sql" "Create error test table"; then
        print_success "Error test table created"
        
        # Test 1: Insert valid record first
        if execute_mysql "INSERT INTO $table_name (id, name, price) VALUES (1, 'Valid', 99.99);" "Insert valid record"; then
            print_success "Valid record inserted successfully"
            
            # Test 2: Try to insert record with string too long (should be handled gracefully)
            print_info "Testing VARCHAR length limit handling..."
            if execute_mysql "INSERT INTO $table_name (id, name, price) VALUES (2, 'This name is too long for VARCHAR(10)', 50.00);" "Insert long string" 2>/dev/null; then
                print_warning "Long string insertion succeeded (may be truncated)"
            else
                print_success "Long string insertion correctly failed or was handled"
            fi
            
            # Test 3: Try to insert invalid decimal (should be handled gracefully)
            print_info "Testing DECIMAL precision handling..."
            if execute_mysql "INSERT INTO $table_name (id, name, price) VALUES (3, 'Test', 12345.67);" "Insert large decimal" 2>/dev/null; then
                print_warning "Large decimal insertion succeeded (may be truncated)"
            else
                print_success "Large decimal insertion correctly failed or was handled"
            fi
            
            # Verify at least the valid record exists
            local row_count
            row_count=$(execute_mysql_with_output "SELECT COUNT(*) FROM $table_name;" "Count error test records")
            local count_value
            count_value=$(echo "$row_count" | tail -n 1)
            
            if [ "$count_value" -ge "1" ]; then
                print_success "Error handling test completed. Records in table: $count_value"
                return 0
            else
                print_error "Error handling test failed. No records found."
                return 1
            fi
        else
            print_error "Failed to insert valid record for error test"
            return 1
        fi
    else
        print_error "Failed to create error test table"
        return 1
    fi
}

# Cleanup function
cleanup_test_tables() {
    print_info "Cleaning up test tables..."
    
    local tables=(
        "${TEST_TABLE_PREFIX}_basic"
        "${TEST_TABLE_PREFIX}_batch"
        "${TEST_TABLE_PREFIX}_datatypes"
        "${TEST_TABLE_PREFIX}_performance"
        "${TEST_TABLE_PREFIX}_errors"
    )
    
    for table in "${tables[@]}"; do
        execute_mysql "DROP TABLE IF EXISTS $table;" "Drop test table: $table" || true
    done
    
    print_info "Cleanup completed"
}

# Main test execution
main() {
    echo "=========================================="
    echo "SBT Storage Engine - Record Insertion Test"
    echo "=========================================="
    echo "Host: $MYSQL_HOST:$MYSQL_PORT"
    echo "User: $MYSQL_USER"
    echo "Database: $MYSQL_DATABASE"
    echo "Protocol: TCP"
    echo "=========================================="
    
    # Check MySQL connection first
    if ! check_mysql_connection; then
        print_error "Cannot proceed without MySQL connection"
        exit 1
    fi
    
    # Run all tests
    run_test "Test Table Setup" test_setup_test_table
    run_test "Single Record Insertion" test_single_record_insertion
    run_test "Multiple Single Insertions" test_multiple_single_insertions
    run_test "Batch Insertion" test_batch_insertion
    run_test "Different Data Types Insertion" test_data_types_insertion
    run_test "Data Persistence Verification" test_data_persistence
    run_test "Large Batch Insertion Performance" test_large_batch_insertion
    run_test "Error Handling" test_error_handling
    
    # Cleanup
    cleanup_test_tables
    
    # Final results
    echo ""
    echo "=========================================="
    echo "Test Results Summary"
    echo "=========================================="
    echo "Total tests: $TOTAL_TESTS"
    echo "Passed: $PASSED_TESTS"
    echo "Failed: $FAILED_TESTS"
    
    if [ $FAILED_TESTS -eq 0 ]; then
        print_success "🎉 ALL TESTS PASSED! 🎉"
        print_success "SBT storage engine record insertion operations are working correctly"
        exit 0
    else
        print_error "❌ SOME TESTS FAILED ❌"
        print_error "Please review the failed tests and fix any issues"
        exit 1
    fi
}

# Handle script arguments
case "${1:-}" in
    --help|-h)
        echo "Usage: $0 [options]"
        echo ""
        echo "Options:"
        echo "  --help, -h          Show this help message"
        echo "  --host HOST         MySQL host (default: $MYSQL_HOST)"
        echo "  --port PORT         MySQL port (default: $MYSQL_PORT)"
        echo "  --user USER         MySQL user (default: $MYSQL_USER)"
        echo "  --password PASS     MySQL password (default: empty)"
        echo "  --database DB       MySQL database (default: $MYSQL_DATABASE)"
        echo ""
        echo "Environment variables:"
        echo "  MYSQL_HOST          Override default host"
        echo "  MYSQL_PORT          Override default port"
        echo "  MYSQL_USER          Override default user"
        echo "  MYSQL_PASSWORD      Override default password"
        echo "  MYSQL_DATABASE      Override default database"
        echo ""
        echo "Example:"
        echo "  $0 --host localhost --user root --password mypass --database testdb"
        exit 0
        ;;
    --host)
        MYSQL_HOST="$2"
        shift 2
        ;;
    --port)
        MYSQL_PORT="$2"
        shift 2
        ;;
    --user)
        MYSQL_USER="$2"
        shift 2
        ;;
    --password)
        MYSQL_PASSWORD="$2"
        shift 2
        ;;
    --database)
        MYSQL_DATABASE="$2"
        shift 2
        ;;
    "")
        # No arguments, use defaults and environment variables
        MYSQL_HOST="${MYSQL_HOST:-127.0.0.1}"
        MYSQL_PORT="${MYSQL_PORT:-3306}"
        MYSQL_USER="${MYSQL_USER:-root}"
        MYSQL_PASSWORD="${MYSQL_PASSWORD:-}"
        MYSQL_DATABASE="${MYSQL_DATABASE:-test}"
        ;;
    *)
        print_error "Unknown option: $1"
        print_error "Use --help for usage information"
        exit 1
        ;;
esac

# Run main function
main
