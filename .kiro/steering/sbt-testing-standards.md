---
inclusion: fileMatch
fileMatchPattern: 'storage/sbt/**'
---

# SBT Storage Engine - Testing Standards

## Overview

This document defines the testing standards and practices for the SBT storage engine project. These guidelines ensure consistent, maintainable, and efficient testing practices.

## Test Organization

### Directory Structure
All tests are organized under `storage/sbt/tests/` with the following structure:

```
storage/sbt/tests/
├── README.md                    # Test documentation
├── Makefile                     # Unified build system
├── standalone/                  # Self-contained test programs
├── integration/                 # Integration and compatibility tests
├── performance/                 # Performance and stress tests
└── gtest/                       # Google Test unit tests for MySQL integration
```

### Test Categories

#### Standalone Tests (`standalone/`)
- **Purpose**: Self-contained test programs that don't depend on MySQL framework
- **Requirements**: C++11 compiler only, no MySQL dependencies
- **Usage**: Direct compilation and execution
- **Example**: `test_insertion_standalone.cc`

#### Integration Tests (`integration/`)
- **Purpose**: Verify compatibility with MySQL framework and existing code
- **Requirements**: Basic MySQL type definitions (can be mocked)
- **Usage**: Test compatibility with existing interfaces
- **Example**: `verify_unit_tests.cc`

#### Performance Tests (`performance/`)
- **Purpose**: Performance, scalability, and stress testing
- **Requirements**: May require longer execution time
- **Usage**: Benchmark and stress test implementations
- **Example**: `test_sbt_advanced.cc`

#### Google Test Unit Tests (`gtest/`)
- **Purpose**: Formal unit tests integrated with MySQL build system
- **Requirements**: Full MySQL build environment, Google Test framework
- **Usage**: Built via MySQL CMake system
- **Integration**: Included in MySQL's unit test suite

## Test Execution Guidelines

### NO Test Runner Scripts
**IMPORTANT**: Do not create wrapper scripts like `run_tests.sh` or similar test runners.

### Direct Test Execution
Tests should be executed directly using one of these methods:

#### For Standalone Tests
```bash
# Build and run directly
make -C storage/sbt/tests test_insertion_standalone

# Or build first, then run
make -C storage/sbt/tests standalone
./storage/sbt/tests/build/test_insertion_standalone
```

#### For Integration Tests
```bash
# Build and run directly
make -C storage/sbt/tests verify_unit_tests

# Or build first, then run
make -C storage/sbt/tests integration
./storage/sbt/tests/build/verify_unit_tests
```

#### For Performance Tests
```bash
# Build and run directly
make -C storage/sbt/tests test_sbt_advanced

# Or build first, then run
make -C storage/sbt/tests performance
./storage/sbt/tests/build/test_sbt_advanced
```

#### For Google Test Unit Tests
```bash
# Built via MySQL build system
cmake --build . --target sbt_unittest
./unittest/sbt_unittest
```

### Batch Test Execution
If multiple tests need to be run, use Makefile targets:

```bash
# Run all standalone tests
make -C storage/sbt/tests run-standalone

# Run all integration tests
make -C storage/sbt/tests run-integration

# Run all performance tests
make -C storage/sbt/tests run-performance
```

## Test Development Standards

### Test File Naming
- **Standalone**: `test_[functionality]_standalone.cc`
- **Integration**: `test_[functionality]_integration.cc` or `verify_[aspect].cc`
- **Performance**: `test_[functionality]_performance.cc` or `test_[functionality]_advanced.cc`
- **Google Test**: `sbt_[module]_test.cc`

### Test Implementation Requirements

#### All Tests Must Include
- Clear, descriptive test names
- Comprehensive error checking
- Both success and failure case testing
- Data integrity verification
- Memory management validation
- Performance considerations (where applicable)

#### Standalone Tests
- No MySQL dependencies
- Self-contained with mocked dependencies
- Comprehensive output with pass/fail indicators
- Return appropriate exit codes (0 for success, non-zero for failure)

#### Integration Tests
- Mock necessary MySQL components minimally
- Test compatibility with existing interfaces
- Verify error handling matches expectations
- Document any MySQL-specific behavior

#### Performance Tests
- Include timing measurements
- Test with realistic data sizes
- Document performance characteristics
- Include memory usage analysis where relevant

#### Google Test Unit Tests
- Use Google Test framework conventions
- Follow MySQL unit test patterns
- Integrate with MySQL build system
- Include in automated test suites

## Build System Integration

### Makefile Standards
- Use unified Makefile in `tests/` directory
- Separate targets for each test category
- Build artifacts in `tests/build/` directory
- Clean target to remove all build artifacts

### CMake Integration
- Google Test unit tests integrated via CMake
- Path references updated for new directory structure
- Conditional compilation based on MySQL build flags

## Continuous Integration

### Test Execution in CI
- **Standalone tests**: Can run without MySQL build environment
- **Integration tests**: Require minimal MySQL components
- **Performance tests**: May be run separately due to execution time
- **Google Test unit tests**: Require full MySQL build environment

### Test Validation
- All tests must pass before task completion
- Test results must be documented in verification files
- Performance characteristics must be within acceptable ranges

## Documentation Requirements

### Test Documentation
Each test category must include:
- Purpose and scope documentation
- Build and execution instructions
- Expected behavior description
- Performance characteristics (for performance tests)

### Verification Integration
- Test results must be included in task verification documents
- Test coverage must be documented
- Any test limitations or known issues must be noted

## Maintenance Guidelines

### Adding New Tests
1. Choose appropriate test category
2. Follow naming conventions
3. Update Makefile if needed
4. Update CMakeLists.txt for Google Test unit tests
5. Document in README.md
6. Include in verification documentation

### Removing Tests
1. Remove source files
2. Update build system files
3. Update documentation
4. Clean up any references in verification documents

### Test Updates
- Keep tests synchronized with implementation changes
- Update expected behavior documentation
- Maintain performance benchmarks
- Ensure compatibility with MySQL framework changes

## Quality Assurance

### Before Task Completion
- All relevant tests must pass
- Test coverage must be adequate
- Performance must meet requirements
- Documentation must be complete
- No test runner scripts should be created

### Code Review
- Test code follows same quality standards as implementation code
- Test logic is clear and maintainable
- Error messages are helpful and descriptive
- Test data is realistic and comprehensive

This testing standard ensures consistent, maintainable, and efficient testing practices across the SBT storage engine project while avoiding unnecessary complexity from wrapper scripts.