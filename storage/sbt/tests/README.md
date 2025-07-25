# SBT Storage Engine Tests

This directory contains all test files for the SBT storage engine, organized by test type and purpose.

## Directory Structure

```
tests/
├── README.md                    # This file
├── Makefile                     # Unified test build system
├── standalone/                  # Self-contained test programs
│   ├── test_insertion_standalone.cc
│   └── test_insertion.cc
├── integration/                 # Integration and compatibility tests
│   ├── verify_unit_tests.cc
│   └── test_sbt_basic.cc
├── performance/                 # Performance and stress tests
│   └── test_sbt_advanced.cc
└── gtest/                       # Google Test unit tests for MySQL integration
    ├── CMakeLists.txt
    ├── sbt_tree_test.cc
    ├── sbt_node_test.cc
    ├── sbt_file_test.cc
    ├── sbt_basic_test.cc
    └── sbt_insertion_test.cc
```

## Test Categories

### Standalone Tests (`standalone/`)
Self-contained test programs that don't depend on MySQL framework:
- **test_insertion_standalone.cc**: Comprehensive insertion testing with SBT balance verification
- **test_insertion.cc**: Original insertion test (deprecated, use standalone version)

### Integration Tests (`integration/`)
Tests that verify compatibility with MySQL framework and existing code:
- **verify_unit_tests.cc**: Verifies compatibility with existing unit test scenarios
- **test_sbt_basic.cc**: Basic integration tests

### Performance Tests (`performance/`)
Tests focused on performance, scalability, and stress testing:
- **test_sbt_advanced.cc**: Advanced performance and stress tests

### Google Test Unit Tests (`gtest/`)
Google Test framework unit tests for MySQL integration:
- **sbt_tree_test.cc**: Core tree functionality tests
- **sbt_node_test.cc**: Node structure and operations tests
- **sbt_file_test.cc**: File operations tests
- **sbt_basic_test.cc**: Basic functionality tests
- **sbt_insertion_test.cc**: Comprehensive insertion tests
- Integrated with MySQL build system
- Uses Google Test framework

## Building and Running Tests

### Quick Test (Standalone)
```bash
cd storage/sbt/tests
make test_insertion_standalone
./test_insertion_standalone
```

### All Standalone Tests
```bash
cd storage/sbt/tests
make standalone
```

### All Integration Tests
```bash
cd storage/sbt/tests
make integration
```

### All Performance Tests
```bash
cd storage/sbt/tests
make performance
```

### Clean Build Artifacts
```bash
cd storage/sbt/tests
make clean
```

## Test Requirements

### For Standalone Tests
- C++11 compiler (g++ or clang++)
- Standard library only
- No MySQL dependencies

### For Integration Tests
- C++11 compiler
- Basic MySQL type definitions (mocked in tests)
- No full MySQL build required

### For Unit Tests
- Full MySQL build environment
- Google Test framework
- MySQL development headers

## Adding New Tests

### Standalone Tests
1. Create test file in `standalone/` directory
2. Add build rule to `Makefile`
3. Follow existing test patterns
4. Include comprehensive error checking

### Integration Tests
1. Create test file in `integration/` directory
2. Mock necessary MySQL components
3. Test compatibility with existing interfaces
4. Verify error handling

### Performance Tests
1. Create test file in `performance/` directory
2. Include timing and memory measurements
3. Test with large datasets
4. Document performance characteristics

### Google Test Unit Tests
1. Create test file in `gtest/` directory
2. Use Google Test framework
3. Follow MySQL unit test conventions
4. Update `gtest/CMakeLists.txt`

## Test Standards

All tests should:
- Have clear, descriptive names
- Include comprehensive error checking
- Test both success and failure cases
- Verify data integrity
- Check memory management
- Document expected behavior
- Include performance considerations

## Continuous Integration

Tests are organized to support different CI scenarios:
- **Standalone tests**: Can run without MySQL build
- **Integration tests**: Require minimal MySQL components
- **Unit tests**: Require full MySQL build environment
- **Performance tests**: May require longer execution time

## Maintenance

- Keep tests up to date with implementation changes
- Remove deprecated test files
- Update documentation when adding new test categories
- Ensure all tests pass before marking tasks complete