# SBT Storage Engine Tests - Unified Build System

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -g -I.. -DMYSQL_SERVER

# Directories
STANDALONE_DIR = standalone
INTEGRATION_DIR = integration
PERFORMANCE_DIR = performance
BUILD_DIR = build

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Standalone tests
STANDALONE_TESTS = test_insertion_standalone test_deletion_standalone test_deletion_advanced
STANDALONE_TARGETS = $(addprefix $(BUILD_DIR)/, $(STANDALONE_TESTS))

$(BUILD_DIR)/test_insertion_standalone: $(STANDALONE_DIR)/test_insertion_standalone.cc | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_deletion_standalone: $(STANDALONE_DIR)/test_deletion_standalone.cc | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_deletion_advanced: $(STANDALONE_DIR)/test_deletion_advanced.cc | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

# Integration tests
INTEGRATION_TESTS = verify_unit_tests test_sbt_basic
INTEGRATION_TARGETS = $(addprefix $(BUILD_DIR)/, $(INTEGRATION_TESTS))

$(BUILD_DIR)/verify_unit_tests: $(INTEGRATION_DIR)/verify_unit_tests.cc | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_sbt_basic: $(INTEGRATION_DIR)/test_sbt_basic.cc | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

# Performance tests
PERFORMANCE_TESTS = test_sbt_advanced
PERFORMANCE_TARGETS = $(addprefix $(BUILD_DIR)/, $(PERFORMANCE_TESTS))

$(BUILD_DIR)/test_sbt_advanced: $(PERFORMANCE_DIR)/test_sbt_advanced.cc | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

# Build targets
.PHONY: all standalone integration performance clean run-standalone run-integration run-performance run-all

all: standalone integration performance

standalone: $(STANDALONE_TARGETS)

integration: $(INTEGRATION_TARGETS)

performance: $(PERFORMANCE_TARGETS)

# Run targets
run-standalone: standalone
	@echo "=== Running Standalone Tests ==="
	@for test in $(STANDALONE_TARGETS); do \
		echo "Running $$test..."; \
		./$$test || exit 1; \
		echo ""; \
	done

run-integration: integration
	@echo "=== Running Integration Tests ==="
	@for test in $(INTEGRATION_TARGETS); do \
		echo "Running $$test..."; \
		./$$test || exit 1; \
		echo ""; \
	done

run-performance: performance
	@echo "=== Running Performance Tests ==="
	@for test in $(PERFORMANCE_TARGETS); do \
		echo "Running $$test..."; \
		./$$test || exit 1; \
		echo ""; \
	done

run-all: run-standalone run-integration run-performance

# Individual test targets (for convenience)
test_insertion_standalone: $(BUILD_DIR)/test_insertion_standalone
	./$(BUILD_DIR)/test_insertion_standalone

test_deletion_standalone: $(BUILD_DIR)/test_deletion_standalone
	./$(BUILD_DIR)/test_deletion_standalone

test_deletion_advanced: $(BUILD_DIR)/test_deletion_advanced
	./$(BUILD_DIR)/test_deletion_advanced

verify_unit_tests: $(BUILD_DIR)/verify_unit_tests
	./$(BUILD_DIR)/verify_unit_tests

test_sbt_basic: $(BUILD_DIR)/test_sbt_basic
	./$(BUILD_DIR)/test_sbt_basic

test_sbt_advanced: $(BUILD_DIR)/test_sbt_advanced
	./$(BUILD_DIR)/test_sbt_advanced

# Clean
clean:
	rm -rf $(BUILD_DIR)
	rm -f *.o *.dSYM

# Help
help:
	@echo "SBT Storage Engine Test Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  all                 - Build all tests"
	@echo "  standalone          - Build standalone tests"
	@echo "  integration         - Build integration tests"
	@echo "  performance         - Build performance tests"
	@echo ""
	@echo "  run-all             - Run all tests"
	@echo "  run-standalone      - Run standalone tests"
	@echo "  run-integration     - Run integration tests"
	@echo "  run-performance     - Run performance tests"
	@echo ""
	@echo "  test_insertion_standalone - Build and run insertion test"
	@echo "  test_deletion_standalone - Build and run deletion test"
	@echo "  test_deletion_advanced - Build and run advanced deletion test"
	@echo "  verify_unit_tests   - Build and run unit test verification"
	@echo "  test_sbt_basic      - Build and run basic tests"
	@echo "  test_sbt_advanced   - Build and run advanced tests"
	@echo ""
	@echo "  clean               - Remove build artifacts"
	@echo "  help                - Show this help message"
	@echo ""
	@echo "Note: Google Test unit tests in gtest/ directory require"
	@echo "      full MySQL build environment and are built via"
	@echo "      the main MySQL CMake system."