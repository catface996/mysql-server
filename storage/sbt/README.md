# SBT Storage Engine

## Overview

The SBT (Size Balanced Tree) Storage Engine is a simplified MySQL storage engine implementation that demonstrates the core concepts of custom storage engine development. It uses a Size Balanced Tree data structure to manage table records and provides basic CRUD operations without primary key or index support.

## Features

- **Basic CRUD Operations**: INSERT, SELECT, UPDATE, DELETE
- **Size Balanced Tree**: Self-balancing binary search tree for data organization
- **Data Persistence**: File-based storage with serialization/deserialization
- **No Primary Keys**: All operations use full table scan for simplicity
- **Thread Safety**: Basic locking mechanisms for concurrent access

## Architecture

The SBT storage engine consists of several key components:

### Core Components

1. **ha_sbt**: MySQL handler interface implementation
2. **SBT_tree**: Size Balanced Tree data structure
3. **SBT_file**: File management for data persistence
4. **SBT_share**: Shared table information management
5. **sbt_common**: Common utilities and error handling

### Directory Structure

```
storage/sbt/
├── include/           # Header files
│   ├── ha_sbt.h      # Handler interface
│   ├── sbt_tree.h    # Tree data structure
│   ├── sbt_file.h    # File management
│   ├── sbt_share.h   # Shared resources
│   └── sbt_common.h  # Common definitions
├── src/               # Source files
│   ├── ha_sbt.cc     # Handler implementation
│   ├── sbt_tree.cc   # Tree implementation
│   ├── sbt_file.cc   # File implementation
│   ├── sbt_share.cc  # Share implementation
│   └── sbt_common.cc # Common utilities
├── unittest/          # Unit tests
├── CMakeLists.txt    # Build configuration
└── README.md         # This file
```

## Building

The SBT storage engine is built as part of the MySQL server build process:

```bash
# Configure MySQL build with SBT engine
mkdir build && cd build
cmake .. -DWITH_DEBUG=1

# Build MySQL with SBT engine
make -j12

# Run unit tests (if enabled)
make test
```

## Usage

Once built and installed, you can use the SBT storage engine in MySQL:

```sql
-- Create a table using SBT engine
CREATE TABLE test_table (
    id INT,
    name VARCHAR(100),
    data TEXT
) ENGINE=SBT;

-- Insert data
INSERT INTO test_table VALUES (1, 'John', 'Sample data');
INSERT INTO test_table VALUES (2, 'Jane', 'More data');

-- Query data
SELECT * FROM test_table;

-- Update data
UPDATE test_table SET name = 'John Doe' WHERE id = 1;

-- Delete data
DELETE FROM test_table WHERE id = 2;

-- Drop table
DROP TABLE test_table;
```

## Limitations

This is a simplified storage engine with several limitations:

- **No Primary Keys**: All record operations use full table scan
- **No Indexes**: No support for secondary indexes
- **No Transactions**: Basic implementation without transaction support
- **No Foreign Keys**: No referential integrity constraints
- **Limited Concurrency**: Basic table-level locking only
- **No Replication**: Limited replication support

## File Format

SBT tables are stored in `.sbt` files with the following format:

```
File Header (64 bytes):
- Magic Number: "SBT\0" (4 bytes)
- Version: 1 (4 bytes)
- Record Count: (8 bytes)
- Next Insert ID: (8 bytes)
- Tree Root Offset: (8 bytes)
- Checksum: (4 bytes)
- Reserved: (32 bytes)

Data Section:
- Serialized SBT tree nodes in pre-order traversal
```

## Development Status

This is a skeleton implementation with the following status:

- ✅ Project structure and build system
- ✅ Core interfaces and data structures
- ✅ Basic MySQL handler integration
- ⏳ SBT tree algorithm implementation (TODO)
- ⏳ File persistence system (TODO)
- ⏳ Comprehensive testing (TODO)

## Testing

Unit tests are provided in the `unittest/` directory:

```bash
# Run SBT unit tests
cd build
./storage/sbt/unittest/sbt_unittest
```

## Contributing

This storage engine is designed for educational purposes and demonstration of MySQL storage engine concepts. Contributions should focus on:

1. Implementing the SBT tree algorithms
2. Adding comprehensive file persistence
3. Improving error handling and logging
4. Adding more comprehensive tests
5. Performance optimizations

## License

This code is licensed under the GPL v2 license, consistent with MySQL server licensing.