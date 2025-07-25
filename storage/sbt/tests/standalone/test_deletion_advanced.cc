#include <iostream>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <algorithm>

// Mock MySQL types for standalone testing
typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long long uint64_t;
typedef uint64_t sbt_insert_id_t;

// SBT error codes
#define SBT_SUCCESS 0
#define SBT_ERR_INVALID_ARGUMENT 1
#define SBT_ERR_OUT_OF_MEMORY 2

// Mock memory management
struct MEM_ROOT {
    static const int PSI_NOT_INSTRUMENTED = 0;
    
    MEM_ROOT(int, size_t) {}
    
    void* Alloc(size_t size) {
        return malloc(size);
    }
    
    void Clear() {}
};

// Mock sbt_data_compare function
int sbt_data_compare(const uchar *data1, uint length1, const uchar *data2, uint length2) {
    if (length1 != length2) {
        return (length1 < length2) ? -1 : 1;
    }
    return memcmp(data1, data2, length1);
}

// SBT Node Structure
struct SBT_node {
  uchar *data;
  uint data_length;
  sbt_insert_id_t insert_id;
  SBT_node *left;
  SBT_node *right;
  uint size;
};

// Simplified SBT Tree Class for testing
class SBT_tree {
private:
  SBT_node *root;
  MEM_ROOT mem_root;
  sbt_insert_id_t next_insert_id;
  uint64_t record_count;

public:
  SBT_tree() : root(nullptr), mem_root(MEM_ROOT::PSI_NOT_INSTRUMENTED, 8192),
               next_insert_id(1), record_count(0) {}

  ~SBT_tree() { clear(); }

  int insert(const uchar *data, uint length) {
    if (!data || length == 0) return SBT_ERR_INVALID_ARGUMENT;
    
    sbt_insert_id_t insert_id = next_insert_id++;
    root = insert_node(root, data, length, insert_id);
    
    if (root) {
      record_count++;
      return SBT_SUCCESS;
    } else {
      next_insert_id--;
      return SBT_ERR_OUT_OF_MEMORY;
    }
  }

  int remove(const uchar *data, uint length) {
    if (!data || length == 0) return SBT_ERR_INVALID_ARGUMENT;
    
    SBT_node *node_to_remove = find_by_data(data, length);
    if (!node_to_remove) return SBT_ERR_INVALID_ARGUMENT;

    root = remove_node(root, data, length);
    if (record_count > 0) record_count--;
    return SBT_SUCCESS;
  }

  SBT_node *find_by_data(const uchar *data, uint length) {
    if (!data || length == 0) return nullptr;
    return find_by_data_recursive(root, data, length);
  }

  SBT_node *get_first() {
    if (!root) return nullptr;
    return find_min(root);
  }

  SBT_node *get_next(SBT_node *current) {
    if (!current) return nullptr;
    if (current->right) return find_min(current->right);
    return find_next_by_insert_id(root, current->insert_id);
  }

  uint64_t get_record_count() const { return record_count; }
  bool is_empty() const { return root == nullptr; }

  void clear() {
    root = nullptr;
    record_count = 0;
    next_insert_id = 1;
    mem_root.Clear();
  }

  // Tree validation methods
  bool is_valid_sbt() const {
    return is_valid_sbt_recursive(root);
  }

  uint get_tree_height() const {
    return get_height_recursive(root);
  }

private:
  SBT_node *create_node(const uchar *data, uint length, sbt_insert_id_t insert_id) {
    if (!data || length == 0) return nullptr;
    
    SBT_node *node = (SBT_node *)mem_root.Alloc(sizeof(SBT_node));
    if (!node) return nullptr;

    node->data = (uchar *)mem_root.Alloc(length);
    if (!node->data) return nullptr;
    memcpy(node->data, data, length);

    node->data_length = length;
    node->insert_id = insert_id;
    node->left = nullptr;
    node->right = nullptr;
    node->size = 1;

    return node;
  }

  SBT_node *insert_node(SBT_node *node, const uchar *data, uint length, sbt_insert_id_t insert_id) {
    if (!node) return create_node(data, length, insert_id);
    
    if (insert_id < node->insert_id) {
      node->left = insert_node(node->left, data, length, insert_id);
    } else {
      node->right = insert_node(node->right, data, length, insert_id);
    }
    
    update_size(node);
    return maintain(node, insert_id >= node->insert_id);
  }

  SBT_node *remove_node(SBT_node *node, const uchar *data, uint length) {
    if (!node) return nullptr;

    if (sbt_data_compare(node->data, node->data_length, data, length) == 0) {
      if (!node->left && !node->right) return nullptr;
      if (!node->left) return node->right;
      if (!node->right) return node->left;
      
      SBT_node *successor = find_min(node->right);
      uchar *new_data = (uchar *)mem_root.Alloc(successor->data_length);
      if (new_data) {
        memcpy(new_data, successor->data, successor->data_length);
        node->data = new_data;
        node->data_length = successor->data_length;
        node->insert_id = successor->insert_id;
      }
      
      node->right = remove_node(node->right, successor->data, successor->data_length);
      update_size(node);
      // After removing from right subtree, maintain both directions
      node = maintain(node, true);
      node = maintain(node, false);
      return node;
    } else {
      node->left = remove_node(node->left, data, length);
      node->right = remove_node(node->right, data, length);
      
      update_size(node);
      // After recursive removal, maintain both directions
      node = maintain(node, false);
      node = maintain(node, true);
      return node;
    }
  }

  SBT_node *maintain(SBT_node *node, bool flag) {
    if (!node) return node;

    if (!flag) {
      // Left subtree was modified
      if (node->left && get_size(node->left->left) > get_size(node->right)) {
        node = rotate_right(node);
      } else if (node->left && get_size(node->left->right) > get_size(node->right)) {
        node->left = rotate_left(node->left);
        node = rotate_right(node);
      } else {
        return node;
      }
    } else {
      // Right subtree was modified
      if (node->right && get_size(node->right->right) > get_size(node->left)) {
        node = rotate_left(node);
      } else if (node->right && get_size(node->right->left) > get_size(node->left)) {
        node->right = rotate_right(node->right);
        node = rotate_left(node);
      } else {
        return node;
      }
    }

    // After rotation, maintain both subtrees
    if (node->left) {
      node->left = maintain(node->left, false);
    }
    if (node->right) {
      node->right = maintain(node->right, true);
    }
    
    return node;
  }

  SBT_node *rotate_left(SBT_node *node) {
    if (!node || !node->right) return node;

    SBT_node *new_root = node->right;
    node->right = new_root->left;
    new_root->left = node;

    update_size(node);
    update_size(new_root);

    return new_root;
  }

  SBT_node *rotate_right(SBT_node *node) {
    if (!node || !node->left) return node;

    SBT_node *new_root = node->left;
    node->left = new_root->right;
    new_root->right = node;

    update_size(node);
    update_size(new_root);

    return new_root;
  }

  void update_size(SBT_node *node) {
    if (node) {
      node->size = 1 + get_size(node->left) + get_size(node->right);
    }
  }

  uint get_size(SBT_node *node) const {
    return node ? node->size : 0;
  }

  SBT_node *find_min(SBT_node *node) {
    if (!node) return nullptr;
    while (node->left) node = node->left;
    return node;
  }

  SBT_node *find_by_data_recursive(SBT_node *node, const uchar *data, uint length) {
    if (!node) return nullptr;
    
    if (sbt_data_compare(node->data, node->data_length, data, length) == 0) {
      return node;
    }

    SBT_node *found = find_by_data_recursive(node->left, data, length);
    if (found) return found;

    return find_by_data_recursive(node->right, data, length);
  }

  SBT_node *find_next_by_insert_id(SBT_node *node, sbt_insert_id_t current_id) {
    if (!node) return nullptr;

    SBT_node *result = nullptr;

    if (node->insert_id > current_id) {
      result = node;
      SBT_node *left_result = find_next_by_insert_id(node->left, current_id);
      if (left_result && left_result->insert_id < result->insert_id) {
        result = left_result;
      }
    } else {
      result = find_next_by_insert_id(node->right, current_id);
    }

    return result;
  }

  // Tree validation helpers
  bool is_valid_sbt_recursive(SBT_node *node) const {
    if (!node) return true;
    
    // Check size property
    uint expected_size = 1 + get_size(node->left) + get_size(node->right);
    if (node->size != expected_size) {
      std::cout << "[DEBUG] Size mismatch at node: expected " << expected_size << ", actual " << node->size << std::endl;
      return false;
    }
    
    // Check SBT balance property
    if (node->left && get_size(node->left->left) > get_size(node->right)) {
      std::cout << "[DEBUG] SBT violation: left->left (" << get_size(node->left->left) << ") > right (" << get_size(node->right) << ")" << std::endl;
      return false;
    }
    if (node->left && get_size(node->left->right) > get_size(node->right)) {
      std::cout << "[DEBUG] SBT violation: left->right (" << get_size(node->left->right) << ") > right (" << get_size(node->right) << ")" << std::endl;
      return false;
    }
    if (node->right && get_size(node->right->right) > get_size(node->left)) {
      std::cout << "[DEBUG] SBT violation: right->right (" << get_size(node->right->right) << ") > left (" << get_size(node->left) << ")" << std::endl;
      return false;
    }
    if (node->right && get_size(node->right->left) > get_size(node->left)) {
      std::cout << "[DEBUG] SBT violation: right->left (" << get_size(node->right->left) << ") > left (" << get_size(node->left) << ")" << std::endl;
      return false;
    }
    
    // Recursively check subtrees
    return is_valid_sbt_recursive(node->left) && is_valid_sbt_recursive(node->right);
  }

  uint get_height_recursive(SBT_node *node) const {
    if (!node) return 0;
    return 1 + std::max(get_height_recursive(node->left), get_height_recursive(node->right));
  }
};

// Test helper functions
void assert_equal(uint64_t expected, uint64_t actual, const char* description) {
    if (expected == actual) {
        std::cout << "[PASS] " << description << std::endl;
    } else {
        std::cout << "[FAIL] " << description << " - Expected: " << expected 
                  << ", Actual: " << actual << std::endl;
        exit(1);
    }
}

void assert_true(bool condition, const char* description) {
    if (condition) {
        std::cout << "[PASS] " << description << std::endl;
    } else {
        std::cout << "[FAIL] " << description << std::endl;
        exit(1);
    }
}

// Advanced test functions
void test_deletion_with_rebalancing() {
    std::cout << "\n=== Deletion with Rebalancing Test ===" << std::endl;
    
    SBT_tree tree;
    
    // Insert records that will create an unbalanced tree after deletions
    std::vector<std::string> records = {
        "record01", "record02", "record03", "record04", "record05",
        "record06", "record07", "record08", "record09", "record10"
    };
    
    // Insert all records
    for (const auto& record : records) {
        int result = tree.insert((const uchar *)record.c_str(), record.length());
        assert_equal(SBT_SUCCESS, result, "Insert record");
    }
    
    assert_equal(records.size(), tree.get_record_count(), "All records inserted");
    assert_true(tree.is_valid_sbt(), "Tree is valid SBT after insertions");
    
    // Delete records in a pattern that would cause imbalance without rebalancing
    std::vector<int> deletion_order = {1, 3, 5, 7, 9, 0, 2, 4, 6, 8};
    
    for (int i : deletion_order) {
        const std::string& record = records[i];
        int result = tree.remove((const uchar *)record.c_str(), record.length());
        assert_equal(SBT_SUCCESS, result, "Delete record");
        assert_true(tree.is_valid_sbt(), "Tree remains valid SBT after deletion");
    }
    
    assert_true(tree.is_empty(), "Tree is empty after all deletions");
}

void test_deletion_performance() {
    std::cout << "\n=== Deletion Performance Test ===" << std::endl;
    
    SBT_tree tree;
    const int num_records = 1000;
    
    // Insert many records
    for (int i = 0; i < num_records; i++) {
        std::string record = "record" + std::to_string(i);
        tree.insert((const uchar *)record.c_str(), record.length());
    }
    
    assert_equal(num_records, tree.get_record_count(), "All records inserted");
    uint initial_height = tree.get_tree_height();
    
    // Delete half the records
    for (int i = 0; i < num_records / 2; i++) {
        std::string record = "record" + std::to_string(i * 2); // Delete even-numbered records
        int result = tree.remove((const uchar *)record.c_str(), record.length());
        assert_equal(SBT_SUCCESS, result, "Delete record");
    }
    
    assert_equal(num_records / 2, tree.get_record_count(), "Half records deleted");
    assert_true(tree.is_valid_sbt(), "Tree remains valid SBT");
    
    uint final_height = tree.get_tree_height();
    
    // Tree height should not increase significantly after deletions
    assert_true(final_height <= initial_height + 2, "Tree height remains reasonable");
    
    std::cout << "[INFO] Initial height: " << initial_height << ", Final height: " << final_height << std::endl;
}

void test_deletion_edge_cases_advanced() {
    std::cout << "\n=== Advanced Deletion Edge Cases Test ===" << std::endl;
    
    SBT_tree tree;
    
    // Test deletion from single-node tree
    const char *single_record = "single";
    tree.insert((const uchar *)single_record, strlen(single_record));
    int result = tree.remove((const uchar *)single_record, strlen(single_record));
    assert_equal(SBT_SUCCESS, result, "Delete from single-node tree");
    assert_true(tree.is_empty(), "Tree is empty after single deletion");
    
    // Test deletion of root with only left child
    tree.clear();
    tree.insert((const uchar *)"root", 4);
    tree.insert((const uchar *)"left", 4);
    result = tree.remove((const uchar *)"root", 4);
    assert_equal(SBT_SUCCESS, result, "Delete root with left child");
    assert_equal(1, tree.get_record_count(), "One record remains");
    assert_true(tree.is_valid_sbt(), "Tree remains valid");
    
    // Test deletion of root with only right child
    tree.clear();
    tree.insert((const uchar *)"root", 4);
    tree.insert((const uchar *)"right", 5);
    result = tree.remove((const uchar *)"root", 4);
    assert_equal(SBT_SUCCESS, result, "Delete root with right child");
    assert_equal(1, tree.get_record_count(), "One record remains");
    assert_true(tree.is_valid_sbt(), "Tree remains valid");
    
    // Test deletion of root with both children
    tree.clear();
    tree.insert((const uchar *)"root", 4);
    tree.insert((const uchar *)"left", 4);
    tree.insert((const uchar *)"right", 5);
    result = tree.remove((const uchar *)"root", 4);
    assert_equal(SBT_SUCCESS, result, "Delete root with both children");
    assert_equal(2, tree.get_record_count(), "Two records remain");
    assert_true(tree.is_valid_sbt(), "Tree remains valid");
}

void test_deletion_and_traversal_consistency() {
    std::cout << "\n=== Deletion and Traversal Consistency Test ===" << std::endl;
    
    SBT_tree tree;
    std::vector<std::string> records = {"aaa", "bbb", "ccc", "ddd", "eee"};
    
    // Insert all records
    for (const auto& record : records) {
        tree.insert((const uchar *)record.c_str(), record.length());
    }
    
    // Delete middle record
    int result = tree.remove((const uchar *)"ccc", 3);
    assert_equal(SBT_SUCCESS, result, "Delete middle record");
    
    // Verify traversal gives correct remaining records
    std::vector<std::string> remaining_records;
    SBT_node *current = tree.get_first();
    while (current) {
        remaining_records.push_back(std::string((char*)current->data, current->data_length));
        current = tree.get_next(current);
    }
    
    assert_equal(4, remaining_records.size(), "Correct number of remaining records");
    
    // Verify deleted record is not in traversal
    bool found_deleted = false;
    for (const auto& record : remaining_records) {
        if (record == "ccc") {
            found_deleted = true;
            break;
        }
    }
    assert_true(!found_deleted, "Deleted record not found in traversal");
    
    // Verify all other records are present
    std::vector<std::string> expected = {"aaa", "bbb", "ddd", "eee"};
    std::sort(remaining_records.begin(), remaining_records.end());
    std::sort(expected.begin(), expected.end());
    
    bool all_present = (remaining_records == expected);
    assert_true(all_present, "All expected records present in traversal");
}

int main() {
    std::cout << "=== SBT Tree Advanced Deletion Test ===" << std::endl;
    
    try {
        test_deletion_with_rebalancing();
        test_deletion_performance();
        test_deletion_edge_cases_advanced();
        test_deletion_and_traversal_consistency();
        
        std::cout << "\n=== All Advanced Deletion Tests Passed! ===" << std::endl;
        return 0;
    } catch (...) {
        std::cout << "\n=== Test Failed with Exception ===" << std::endl;
        return 1;
    }
}