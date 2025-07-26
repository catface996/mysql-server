#!/bin/bash

# SBT Storage Engine - Quick Table Operations Test
# Simple test script for basic table creation and deletion

set -e

# Configuration
MYSQL_CMD="mysql -u root test"
TEST_TABLE="sbt_quick_test"

echo "=== SBT Storage Engine Quick Table Test ==="

# Test 1: Check if SBT engine is available
echo "1. Checking SBT engine availability..."
if $MYSQL_CMD -e "SHOW ENGINES;" | grep -q "SBT"; then
    echo "✓ SBT engine is available"
else
    echo "✗ SBT engine is not available"
    exit 1
fi

# Test 2: Create table with SBT engine
echo "2. Creating table with SBT engine..."
$MYSQL_CMD -e "DROP TABLE IF EXISTS $TEST_TABLE;"
if $MYSQL_CMD -e "CREATE TABLE $TEST_TABLE (id INT, name VARCHAR(50)) ENGINE=SBT;"; then
    echo "✓ Table created successfully"
else
    echo "✗ Failed to create table"
    exit 1
fi

# Test 3: Verify table uses SBT engine
echo "3. Verifying table engine..."
if $MYSQL_CMD -e "SHOW CREATE TABLE $TEST_TABLE;" | grep -q "ENGINE=SBT"; then
    echo "✓ Table correctly uses SBT engine"
else
    echo "✗ Table does not use SBT engine"
    exit 1
fi

# Test 4: Drop table
echo "4. Dropping table..."
if $MYSQL_CMD -e "DROP TABLE $TEST_TABLE;"; then
    echo "✓ Table dropped successfully"
else
    echo "✗ Failed to drop table"
    exit 1
fi

# Test 5: Verify table is gone
echo "5. Verifying table deletion..."
if ! $MYSQL_CMD -e "SHOW TABLES LIKE '$TEST_TABLE';" | grep -q "$TEST_TABLE"; then
    echo "✓ Table successfully deleted"
else
    echo "✗ Table still exists after drop"
    exit 1
fi

echo "🎉 All quick tests passed!"
echo "SBT storage engine table operations are working correctly."