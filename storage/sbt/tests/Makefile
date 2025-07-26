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
STANDALONE_TESTS = test_insertion_standalone test_deletion_standalone test_deletion_advanced test_search_traversal_standalone test_traversal_order_standalone test_sbt_share_standalone test_task_4_2_verification test_ha_sbt_structure test_task_5_1_verification test_task_5_4_verification test_table_create_delete test_table_open_close test_update_standalone test_ha_sbt_update test_error_handling_standalone
STANDALONE_TARGETS = $(addprefix $(BUILD_DIR)/, $(STANDALONE_TESTS))

$(BUILD_DIR)/test_insertion_standalone: $(STANDALONE_DIR)/test_insertion_standalone.cc | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_deletion_standalone: $(STANDALONE_DIR)/test_deletion_standalone.cc | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_deletion_advanced: $(STANDALONE_DIR)/test_deletion_advanced.cc | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_search_traversal_standalone: $(STANDALONE_DIR)/test_search_traversal_standalone.cc | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_traversal_order_standalone: $(STANDALONE_DIR)/test_traversal_order_standalone.cc | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_sbt_share_standalone: $(STANDALONE_DIR)/test_sbt_share_standalone.cc | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -pthread -o $@ $<

$(BUILD_DIR)/test_task_4_2_verification: $(STANDALONE_DIR)/test_task_4_2_verification.cc | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -pthread -o $@ $<

$(BUILD_DIR)/test_ha_sbt_structure: $(STANDALONE_DIR)/test_ha_sbt_structure.cc | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_task_5_1_verification: $(STANDALONE_DIR)/test_task_5_1_verification.cc | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_task_5_4_verification: $(STANDALONE_DIR)/test_task_5_4_verification.cc | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_table_create_delete: $(STANDALONE_DIR)/test_table_create_delete.cc | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_table_open_close: $(STANDALONE_DIR)/test_table_open_close.cc | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_update_standalone: $(STANDALONE_DIR)/test_update_standalone.cc | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_ha_sbt_update: $(STANDALONE_DIR)/test_ha_sbt_update.cc | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_error_handling_standalone: $(STANDALONE_DIR)/test_error_handling_standalone.cc | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

# Integration tests
INTEGRATION_TESTS = verify_unit_tests test_sbt_basic test_search_traversal_integration test_ha_sbt_basic test_ha_sbt_table_ops test_table_corruption_recovery
INTEGRATION_TARGETS = $(addprefix $(BUILD_DIR)/, $(INTEGRATION_TESTS))

$(BUILD_DIR)/verify_unit_tests: $(INTEGRATION_DIR)/verify_unit_tests.cc | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_sbt_basic: $(INTEGRATION_DIR)/test_sbt_basic.cc | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_search_traversal_integration: $(INTEGRATION_DIR)/test_search_traversal_integration.cc | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_ha_sbt_basic: $(INTEGRATION_DIR)/test_ha_sbt_basic.cc | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_ha_sbt_table_ops: $(INTEGRATION_DIR)/test_ha_sbt_table_ops.cc | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_table_corruption_recovery: $(INTEGRATION_DIR)/test_table_corruption_recovery.cc | $(BUILD_DIR)
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

test_search_traversal_standalone: $(BUILD_DIR)/test_search_traversal_standalone
	./$(BUILD_DIR)/test_search_traversal_standalone

test_traversal_order_standalone: $(BUILD_DIR)/test_traversal_order_standalone
	./$(BUILD_DIR)/test_traversal_order_standalone

test_search_traversal_integration: $(BUILD_DIR)/test_search_traversal_integration
	./$(BUILD_DIR)/test_search_traversal_integration

test_sbt_share_standalone: $(BUILD_DIR)/test_sbt_share_standalone
	./$(BUILD_DIR)/test_sbt_share_standalone

test_task_4_2_verification: $(BUILD_DIR)/test_task_4_2_verification
	./$(BUILD_DIR)/test_task_4_2_verification

test_update_standalone: $(BUILD_DIR)/test_update_standalone
	./$(BUILD_DIR)/test_update_standalone

test_ha_sbt_update: $(BUILD_DIR)/test_ha_sbt_update
	./$(BUILD_DIR)/test_ha_sbt_update

test_ha_sbt_basic: $(BUILD_DIR)/test_ha_sbt_basic
	./$(BUILD_DIR)/test_ha_sbt_basic

test_ha_sbt_table_ops: $(BUILD_DIR)/test_ha_sbt_table_ops
	./$(BUILD_DIR)/test_ha_sbt_table_ops

test_table_corruption_recovery: $(BUILD_DIR)/test_table_corruption_recovery
	./$(BUILD_DIR)/test_table_corruption_recovery

test_ha_sbt_structure: $(BUILD_DIR)/test_ha_sbt_structure
	./$(BUILD_DIR)/test_ha_sbt_structure

test_task_5_1_verification: $(BUILD_DIR)/test_task_5_1_verification
	./$(BUILD_DIR)/test_task_5_1_verification

test_task_5_4_verification: $(BUILD_DIR)/test_task_5_4_verification
	./$(BUILD_DIR)/test_task_5_4_verification

test_table_create_delete: $(BUILD_DIR)/test_table_create_delete
	./$(BUILD_DIR)/test_table_create_delete

test_table_open_close: $(BUILD_DIR)/test_table_open_close
	./$(BUILD_DIR)/test_table_open_close

test_error_handling_standalone: $(BUILD_DIR)/test_error_handling_standalone
	./$(BUILD_DIR)/test_error_handling_standalone

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
	@echo "  test_search_traversal_standalone - Build and run search/traversal test"
	@echo "  test_traversal_order_standalone - Build and run traversal order test"
	@echo "  verify_unit_tests   - Build and run unit test verification"
	@echo "  test_sbt_basic      - Build and run basic tests"
	@echo "  test_sbt_advanced   - Build and run advanced tests"
	@echo "  test_search_traversal_integration - Build and run search/traversal integration test"
	@echo "  test_sbt_share_standalone - Build and run SBT share management test"
	@echo ""
	@echo "  clean               - Remove build artifacts"
	@echo "  help                - Show this help message"
	@echo ""
	@echo "Note: Google Test unit tests in gtest/ directory require"
	@echo "      full MySQL build environment and are built via"
	@echo "      the main MySQL CMake system."