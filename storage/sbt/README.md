# SBT (Size Balanced Tree) Storage Engine

## Overview

The SBT storage engine is a MySQL storage engine implementation based on the Size Balanced Tree (SBT) data structure. This is a learning implementation that demonstrates how to create a custom storage engine for MySQL.

## Features

- Size Balanced Tree data structure for efficient operations
- Basic CRUD operations (CREATE, READ, UPDATE, DELETE)
- Table-level locking
- Disk persistence
- No primary key or index support (simplified implementation)

## Architecture

### Core Components

1. **ha_sbt** - Main handler class implementing MySQL's storage engine interface
2. **SBT_tree** - Size Balanced Tree implementation with balancing algorithms
3. **SBT_share** - Shared data structure management with reference counting
4. **SBT_storage** - Disk I/O and persistence layer
5. **SBT_common** - Utility functions and error handling

### File Structure

```
storage/sbt/
├── ha_sbt.h/.cc          # Main handler implementation
├── sbt_tree.h/.cc        # SBT data structure and algorithms
├── sbt_share.h/.cc       # Shared resource management
├── sbt_storage.h/.cc     # Disk storage and I/O
├── sbt_common.h/.cc      # Common utilities and error handling
├── CMakeLists.txt        # Build configuration
└── README.md             # This file
```

## Building

The SBT storage engine is built as part of the MySQL server build process:

```bash
# From MySQL source root
mkdir build && cd build
cmake .. -DWITH_DEBUG=1
make -j$(nproc)
```

## Installation

After building, install the plugin:

```sql
INSTALL PLUGIN sbt SONAME 'ha_sbt.so';
```

## Usage

Create a table using the SBT storage engine:

```sql
CREATE TABLE test_table (
    id INT,
    name VARCHAR(100),
    data TEXT
) ENGINE=SBT;
```

## Current Status

This is the initial project structure setup. The following components are implemented as stubs:

- [x] Project directory structure
- [x] Header file interfaces
- [x] CMake build configuration
- [x] Basic handler class structure
- [x] Plugin registration framework
- [ ] SBT tree algorithms (TODO)
- [ ] CRUD operations (TODO)
- [ ] Disk persistence (TODO)
- [ ] Error handling (TODO)

## Development Notes

- All source files include proper MySQL copyright headers
- Debug macros are used throughout for debugging support
- Error handling follows MySQL conventions
- Memory management uses safe allocation wrappers
- Thread safety is considered in shared resource management

## Next Steps

The next tasks in the implementation plan are:

1. Implement SBT core data structures and algorithms
2. Add CRUD operations to the tree
3. Implement disk storage and persistence
4. Add proper error handling and logging
5. Create comprehensive test suite

## License

This code is licensed under the GPL v2 license, consistent with MySQL server licensing.