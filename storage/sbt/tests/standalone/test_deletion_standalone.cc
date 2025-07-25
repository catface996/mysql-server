#include <iostream>
#include <cstring>
#include <cstdlib>

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

  uint64_t get_record_count() const { return record_count; }
  bool is_empty() const { return root == nullptr; }

  void clear() {
    root = nullptr;
    record_count = 0;
    next_insert_id = 1;
    mem_root.Clear();
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
    return node;
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
    } else {
      node->left = remove_node(node->left, data, length);
      node->right = remove_node(node->right, data, length);
    }

    update_size(node);
    return node;
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

  void update_size(SBT_node *node) {
    if (node) {
      node->size = 1 + get_size(node->left) + get_size(node->right);
    }
  }

  uint get_size(SBT_node *node) const {
    return node ? node->size : 0;
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

void assert_null(void* ptr, const char* description) {
    if (ptr == nullptr) {
        std::cout << "[PASS] " << description << std::endl;
    } else {
        std::cout << "[FAIL] " << description << " - Expected null pointer" << std::endl;
        exit(1);
    }
}

void assert_not_null(void* ptr, const char* description) {
    if (ptr != nullptr) {
        std::cout << "[PASS] " << description << std::endl;
    } else {
        std::cout << "[FAIL] " << description << " - Expected non-null pointer" << std::endl;
        exit(1);
    }
}

// Test functions
void test_delete_single_record() {
    std::cout << "\n=== Delete Single Record Test ===" << std::endl;
    
    SBT_tree tree;
    const char *data = "test_record";
    
    int result = tree.insert((const uchar *)data, strlen(data));
    assert_equal(SBT_SUCCESS, result, "Insert record");
    assert_equal(1, tree.get_record_count(), "Record count after insert");
    
    SBT_node *found = tree.find_by_data((const uchar *)data, strlen(data));
    assert_not_null(found, "Find record before deletion");
    
    result = tree.remove((const uchar *)data, strlen(data));
    assert_equal(SBT_SUCCESS, result, "Delete record");
    assert_equal(0, tree.get_record_count(), "Record count after deletion");
    
    found = tree.find_by_data((const uchar *)data, strlen(data));
    assert_null(found, "Record not found after deletion");
    
    assert_true(tree.is_empty(), "Tree is empty after deletion");
}

void test_delete_nonexistent_record() {
    std::cout << "\n=== Delete Nonexistent Record Test ===" << std::endl;
    
    SBT_tree tree;
    const char *data1 = "existing_record";
    const char *data2 = "nonexistent_record";
    
    int result = tree.insert((const uchar *)data1, strlen(data1));
    assert_equal(SBT_SUCCESS, result, "Insert existing record");
    
    result = tree.remove((const uchar *)data2, strlen(data2));
    assert_equal(SBT_ERR_INVALID_ARGUMENT, result, "Delete nonexistent record returns error");
    
    assert_equal(1, tree.get_record_count(), "Record count unchanged");
    SBT_node *found = tree.find_by_data((const uchar *)data1, strlen(data1));
    assert_not_null(found, "Original record still exists");
}

void test_delete_multiple_records() {
    std::cout << "\n=== Delete Multiple Records Test ===" << std::endl;
    
    SBT_tree tree;
    const char *data1 = "record1";
    const char *data2 = "record2";
    const char *data3 = "record3";
    
    tree.insert((const uchar *)data1, strlen(data1));
    tree.insert((const uchar *)data2, strlen(data2));
    tree.insert((const uchar *)data3, strlen(data3));
    assert_equal(3, tree.get_record_count(), "Initial record count");
    
    int result = tree.remove((const uchar *)data2, strlen(data2));
    assert_equal(SBT_SUCCESS, result, "Delete middle record");
    assert_equal(2, tree.get_record_count(), "Record count after first deletion");
    
    SBT_node *found = tree.find_by_data((const uchar *)data2, strlen(data2));
    assert_null(found, "Deleted record not found");
    
    found = tree.find_by_data((const uchar *)data1, strlen(data1));
    assert_not_null(found, "First record still exists");
    found = tree.find_by_data((const uchar *)data3, strlen(data3));
    assert_not_null(found, "Third record still exists");
    
    result = tree.remove((const uchar *)data1, strlen(data1));
    assert_equal(SBT_SUCCESS, result, "Delete first record");
    assert_equal(1, tree.get_record_count(), "Record count after second deletion");
    
    result = tree.remove((const uchar *)data3, strlen(data3));
    assert_equal(SBT_SUCCESS, result, "Delete last record");
    assert_equal(0, tree.get_record_count(), "Record count after all deletions");
    
    assert_true(tree.is_empty(), "Tree is empty after all deletions");
}

void test_delete_edge_cases() {
    std::cout << "\n=== Delete Edge Cases Test ===" << std::endl;
    
    SBT_tree tree;
    
    const char *data = "test_record";
    int result = tree.remove((const uchar *)data, strlen(data));
    assert_equal(SBT_ERR_INVALID_ARGUMENT, result, "Delete from empty tree returns error");
    
    result = tree.remove(nullptr, 0);
    assert_equal(SBT_ERR_INVALID_ARGUMENT, result, "Delete null data returns error");
    
    result = tree.remove((const uchar *)data, 0);
    assert_equal(SBT_ERR_INVALID_ARGUMENT, result, "Delete zero length returns error");
}

int main() {
    std::cout << "=== SBT Tree Deletion Operations Test ===" << std::endl;
    
    try {
        test_delete_single_record();
        test_delete_nonexistent_record();
        test_delete_multiple_records();
        test_delete_edge_cases();
        
        std::cout << "\n=== All Deletion Tests Passed! ===" << std::endl;
        return 0;
    } catch (...) {
        std::cout << "\n=== Test Failed with Exception ===" << std::endl;
        return 1;
    }
}