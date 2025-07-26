/**
 * @file test_update_standalone.cc
 * @brief Standalone test for SBT tree update operations
 * 
 * This test verifies the update functionality of SBT trees without
 * requiring MySQL framework dependencies.
 */

#include <iostream>
#include <cstring>
#include <cassert>
#include <vector>
#include <string>

// Mock MySQL types for standalone testing
typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long long sbt_insert_id_t;

// SBT error codes
#define SBT_SUCCESS 0
#define SBT_ERR_INVALID_ARGUMENT 1
#define SBT_ERR_OUT_OF_MEMORY 2
#define SBT_ERR_NOT_FOUND 3

// Memory management functions
void* sbt_malloc(size_t size) {
    return malloc(size);
}

void sbt_free(void* ptr) {
    if (ptr) free(ptr);
}

// SBT node structure
struct SBT_node {
    uchar *data;
    uint length;
    sbt_insert_id_t insert_id;
    uint size;
    SBT_node *left;
    SBT_node *right;
    
    SBT_node() : data(nullptr), length(0), insert_id(0), size(1), left(nullptr), right(nullptr) {}
};

// SBT tree class for testing
class SBT_tree {
private:
    SBT_node *root;
    sbt_insert_id_t next_insert_id;

public:
    SBT_tree() : root(nullptr), next_insert_id(1) {}
    
    ~SBT_tree() {
        destroy_tree(root);
    }
    
    // Core operations
    int insert(const uchar *data, uint length);
    int remove(const uchar *data, uint length);
    int update(const uchar *old_data, uint old_length,
               const uchar *new_data, uint new_length);
    
    // Search operations
    SBT_node *find_by_data(const uchar *data, uint length);
    SBT_node *get_first();
    SBT_node *get_next(SBT_node *current);
    
    // Utility functions
    uint get_count() const { return get_size(root); }
    bool is_empty() const { return root == nullptr; }
    
private:
    // Helper functions
    void destroy_tree(SBT_node *node);
    SBT_node *create_node(const uchar *data, uint length, sbt_insert_id_t insert_id);
    SBT_node *insert_node(SBT_node *node, const uchar *data, uint length, sbt_insert_id_t insert_id);
    SBT_node *remove_node(SBT_node *node, const uchar *data, uint length);
    SBT_node *find_by_data_recursive(SBT_node *node, const uchar *data, uint length);
    SBT_node *find_min(SBT_node *node);
    SBT_node *find_next_by_insert_id(SBT_node *node, sbt_insert_id_t current_id);
    
    // SBT operations
    SBT_node *left_rotate(SBT_node *node);
    SBT_node *right_rotate(SBT_node *node);
    SBT_node *maintain(SBT_node *node, bool flag);
    void update_size(SBT_node *node);
    uint get_size(SBT_node *node) const;
    
    // Data comparison
    int compare_data(const uchar *data1, uint len1, const uchar *data2, uint len2);
};

// Implementation of core methods
void SBT_tree::destroy_tree(SBT_node *node) {
    if (node) {
        destroy_tree(node->left);
        destroy_tree(node->right);
        if (node->data) sbt_free(node->data);
        delete node;
    }
}

SBT_node *SBT_tree::create_node(const uchar *data, uint length, sbt_insert_id_t insert_id) {
    SBT_node *node = new SBT_node();
    if (!node) return nullptr;
    
    node->data = (uchar*)sbt_malloc(length);
    if (!node->data) {
        delete node;
        return nullptr;
    }
    
    memcpy(node->data, data, length);
    node->length = length;
    node->insert_id = insert_id;
    node->size = 1;
    node->left = nullptr;
    node->right = nullptr;
    
    return node;
}

int SBT_tree::compare_data(const uchar *data1, uint len1, const uchar *data2, uint len2) {
    uint min_len = (len1 < len2) ? len1 : len2;
    int result = memcmp(data1, data2, min_len);
    if (result == 0) {
        if (len1 < len2) return -1;
        if (len1 > len2) return 1;
        return 0;
    }
    return result;
}

uint SBT_tree::get_size(SBT_node *node) const {
    return node ? node->size : 0;
}

void SBT_tree::update_size(SBT_node *node) {
    if (node) {
        node->size = 1 + get_size(node->left) + get_size(node->right);
    }
}

SBT_node *SBT_tree::left_rotate(SBT_node *node) {
    SBT_node *right = node->right;
    node->right = right->left;
    right->left = node;
    update_size(node);
    update_size(right);
    return right;
}

SBT_node *SBT_tree::right_rotate(SBT_node *node) {
    SBT_node *left = node->left;
    node->left = left->right;
    left->right = node;
    update_size(node);
    update_size(left);
    return left;
}

SBT_node *SBT_tree::maintain(SBT_node *node, bool flag) {
    if (!node) return node;
    
    if (!flag) {
        if (node->left && node->left->left && 
            get_size(node->left->left) > get_size(node->right)) {
            node = right_rotate(node);
        } else if (node->left && node->left->right &&
                   get_size(node->left->right) > get_size(node->right)) {
            node->left = left_rotate(node->left);
            node = right_rotate(node);
        } else {
            return node;
        }
    } else {
        if (node->right && node->right->right &&
            get_size(node->right->right) > get_size(node->left)) {
            node = left_rotate(node);
        } else if (node->right && node->right->left &&
                   get_size(node->right->left) > get_size(node->left)) {
            node->right = right_rotate(node->right);
            node = left_rotate(node);
        } else {
            return node;
        }
    }
    
    node->left = maintain(node->left, false);
    node->right = maintain(node->right, true);
    node = maintain(node, false);
    node = maintain(node, true);
    
    return node;
}

int SBT_tree::insert(const uchar *data, uint length) {
    if (!data || length == 0) {
        return SBT_ERR_INVALID_ARGUMENT;
    }
    
    root = insert_node(root, data, length, next_insert_id++);
    return root ? SBT_SUCCESS : SBT_ERR_OUT_OF_MEMORY;
}

SBT_node *SBT_tree::insert_node(SBT_node *node, const uchar *data, uint length, sbt_insert_id_t insert_id) {
    if (!node) {
        return create_node(data, length, insert_id);
    }
    
    if (insert_id < node->insert_id) {
        node->left = insert_node(node->left, data, length, insert_id);
    } else {
        node->right = insert_node(node->right, data, length, insert_id);
    }
    
    update_size(node);
    return maintain(node, insert_id >= node->insert_id);
}

int SBT_tree::remove(const uchar *data, uint length) {
    if (!data || length == 0) {
        return SBT_ERR_INVALID_ARGUMENT;
    }
    
    SBT_node *old_root = root;
    root = remove_node(root, data, length);
    
    return (old_root != root || !old_root) ? SBT_SUCCESS : SBT_ERR_NOT_FOUND;
}

SBT_node *SBT_tree::remove_node(SBT_node *node, const uchar *data, uint length) {
    if (!node) return nullptr;
    
    int cmp = compare_data(data, length, node->data, node->length);
    if (cmp == 0) {
        if (!node->left || !node->right) {
            SBT_node *temp = node->left ? node->left : node->right;
            if (node->data) sbt_free(node->data);
            delete node;
            return temp;
        } else {
            SBT_node *min_node = find_min(node->right);
            
            if (node->data) sbt_free(node->data);
            node->data = (uchar*)sbt_malloc(min_node->length);
            memcpy(node->data, min_node->data, min_node->length);
            node->length = min_node->length;
            node->insert_id = min_node->insert_id;
            
            node->right = remove_node(node->right, min_node->data, min_node->length);
        }
    } else {
        if (cmp < 0) {
            node->left = remove_node(node->left, data, length);
        } else {
            node->right = remove_node(node->right, data, length);
        }
    }
    
    update_size(node);
    return node;
}

SBT_node *SBT_tree::find_min(SBT_node *node) {
    while (node && node->left) {
        node = node->left;
    }
    return node;
}

SBT_node *SBT_tree::find_by_data(const uchar *data, uint length) {
    if (!data || length == 0) return nullptr;
    return find_by_data_recursive(root, data, length);
}

SBT_node *SBT_tree::find_by_data_recursive(SBT_node *node, const uchar *data, uint length) {
    if (!node) return nullptr;
    
    if (node->length == length && memcmp(node->data, data, length) == 0) {
        return node;
    }
    
    SBT_node *found = find_by_data_recursive(node->left, data, length);
    if (found) return found;
    
    return find_by_data_recursive(node->right, data, length);
}

SBT_node *SBT_tree::get_first() {
    if (!root) return nullptr;
    
    SBT_node *first = nullptr;
    sbt_insert_id_t min_id = ULLONG_MAX;
    
    // Find node with minimum insert_id
    std::function<void(SBT_node*)> find_min_id = [&](SBT_node *node) {
        if (!node) return;
        
        if (node->insert_id < min_id) {
            min_id = node->insert_id;
            first = node;
        }
        
        find_min_id(node->left);
        find_min_id(node->right);
    };
    
    find_min_id(root);
    return first;
}

SBT_node *SBT_tree::get_next(SBT_node *current) {
    if (!current) return nullptr;
    return find_next_by_insert_id(root, current->insert_id);
}

SBT_node *SBT_tree::find_next_by_insert_id(SBT_node *node, sbt_insert_id_t current_id) {
    if (!node) return nullptr;
    
    SBT_node *next = nullptr;
    sbt_insert_id_t min_next_id = ULLONG_MAX;
    
    std::function<void(SBT_node*)> find_next = [&](SBT_node *n) {
        if (!n) return;
        
        if (n->insert_id > current_id && n->insert_id < min_next_id) {
            min_next_id = n->insert_id;
            next = n;
        }
        
        find_next(n->left);
        find_next(n->right);
    };
    
    find_next(node);
    return next;
}

// Update method implementation
int SBT_tree::update(const uchar *old_data, uint old_length,
                     const uchar *new_data, uint new_length) {
    if (!old_data || !new_data || old_length == 0 || new_length == 0) {
        return SBT_ERR_INVALID_ARGUMENT;
    }

    // Find the node with old data
    SBT_node *node = find_by_data(old_data, old_length);
    if (!node) {
        return SBT_ERR_NOT_FOUND; // Record not found
    }

    // If lengths match, update in place
    if (old_length == new_length) {
        memcpy(node->data, new_data, new_length);
        return SBT_SUCCESS;
    }

    // If lengths don't match, we need to reallocate
    uchar *new_data_copy = (uchar*)sbt_malloc(new_length);
    if (!new_data_copy) {
        return SBT_ERR_OUT_OF_MEMORY;
    }
    
    memcpy(new_data_copy, new_data, new_length);
    
    // Free old data and update
    sbt_free(node->data);
    node->data = new_data_copy;
    node->length = new_length;
    
    return SBT_SUCCESS;
}

// Test helper functions
void print_test_header(const std::string& test_name) {
    std::cout << "\n=== " << test_name << " ===" << std::endl;
}

void print_test_result(const std::string& test_name, bool passed) {
    std::cout << test_name << ": " << (passed ? "PASSED" : "FAILED") << std::endl;
}

// Test functions
bool test_basic_update() {
    print_test_header("Basic Update Test");
    
    SBT_tree tree;
    
    // Insert some test data
    std::string data1 = "Hello";
    std::string data2 = "World";
    std::string data3 = "Test";
    
    int result = tree.insert((uchar*)data1.c_str(), data1.length());
    if (result != SBT_SUCCESS) {
        std::cout << "Failed to insert data1" << std::endl;
        return false;
    }
    
    result = tree.insert((uchar*)data2.c_str(), data2.length());
    if (result != SBT_SUCCESS) {
        std::cout << "Failed to insert data2" << std::endl;
        return false;
    }
    
    result = tree.insert((uchar*)data3.c_str(), data3.length());
    if (result != SBT_SUCCESS) {
        std::cout << "Failed to insert data3" << std::endl;
        return false;
    }
    
    std::cout << "Initial tree size: " << tree.get_count() << std::endl;
    
    // Test update with same length
    std::string new_data1 = "Hi!!!";
    result = tree.update((uchar*)data1.c_str(), data1.length(),
                        (uchar*)new_data1.c_str(), new_data1.length());
    
    if (result != SBT_SUCCESS) {
        std::cout << "Failed to update data1 with same length" << std::endl;
        return false;
    }
    
    // Verify the update
    SBT_node *found = tree.find_by_data((uchar*)new_data1.c_str(), new_data1.length());
    if (!found) {
        std::cout << "Updated data not found" << std::endl;
        return false;
    }
    
    // Verify old data is gone
    SBT_node *old_found = tree.find_by_data((uchar*)data1.c_str(), data1.length());
    if (old_found) {
        std::cout << "Old data still exists after update" << std::endl;
        return false;
    }
    
    std::cout << "Tree size after update: " << tree.get_count() << std::endl;
    
    return true;
}

bool test_update_different_length() {
    print_test_header("Update Different Length Test");
    
    SBT_tree tree;
    
    // Insert test data
    std::string data1 = "Short";
    std::string data2 = "Medium length";
    
    tree.insert((uchar*)data1.c_str(), data1.length());
    tree.insert((uchar*)data2.c_str(), data2.length());
    
    std::cout << "Initial tree size: " << tree.get_count() << std::endl;
    
    // Update with different length
    std::string new_data = "This is a much longer string than the original";
    int result = tree.update((uchar*)data1.c_str(), data1.length(),
                            (uchar*)new_data.c_str(), new_data.length());
    
    if (result != SBT_SUCCESS) {
        std::cout << "Failed to update with different length" << std::endl;
        return false;
    }
    
    // Verify the update
    SBT_node *found = tree.find_by_data((uchar*)new_data.c_str(), new_data.length());
    if (!found) {
        std::cout << "Updated data not found" << std::endl;
        return false;
    }
    
    if (found->length != new_data.length()) {
        std::cout << "Updated data length mismatch" << std::endl;
        return false;
    }
    
    // Verify old data is gone
    SBT_node *old_found = tree.find_by_data((uchar*)data1.c_str(), data1.length());
    if (old_found) {
        std::cout << "Old data still exists after update" << std::endl;
        return false;
    }
    
    std::cout << "Tree size after update: " << tree.get_count() << std::endl;
    
    return true;
}

bool test_update_nonexistent() {
    print_test_header("Update Nonexistent Record Test");
    
    SBT_tree tree;
    
    // Insert some data
    std::string data1 = "Exists";
    tree.insert((uchar*)data1.c_str(), data1.length());
    
    // Try to update nonexistent data
    std::string nonexistent = "DoesNotExist";
    std::string new_data = "NewValue";
    
    int result = tree.update((uchar*)nonexistent.c_str(), nonexistent.length(),
                            (uchar*)new_data.c_str(), new_data.length());
    
    if (result != SBT_ERR_NOT_FOUND) {
        std::cout << "Expected NOT_FOUND error, got: " << result << std::endl;
        return false;
    }
    
    std::cout << "Correctly returned NOT_FOUND for nonexistent record" << std::endl;
    
    return true;
}

bool test_update_invalid_arguments() {
    print_test_header("Update Invalid Arguments Test");
    
    SBT_tree tree;
    
    std::string data = "Test";
    std::string new_data = "New";
    
    // Test null old_data
    int result = tree.update(nullptr, data.length(),
                            (uchar*)new_data.c_str(), new_data.length());
    if (result != SBT_ERR_INVALID_ARGUMENT) {
        std::cout << "Expected INVALID_ARGUMENT for null old_data" << std::endl;
        return false;
    }
    
    // Test null new_data
    result = tree.update((uchar*)data.c_str(), data.length(),
                        nullptr, new_data.length());
    if (result != SBT_ERR_INVALID_ARGUMENT) {
        std::cout << "Expected INVALID_ARGUMENT for null new_data" << std::endl;
        return false;
    }
    
    // Test zero old_length
    result = tree.update((uchar*)data.c_str(), 0,
                        (uchar*)new_data.c_str(), new_data.length());
    if (result != SBT_ERR_INVALID_ARGUMENT) {
        std::cout << "Expected INVALID_ARGUMENT for zero old_length" << std::endl;
        return false;
    }
    
    // Test zero new_length
    result = tree.update((uchar*)data.c_str(), data.length(),
                        (uchar*)new_data.c_str(), 0);
    if (result != SBT_ERR_INVALID_ARGUMENT) {
        std::cout << "Expected INVALID_ARGUMENT for zero new_length" << std::endl;
        return false;
    }
    
    std::cout << "All invalid argument tests passed" << std::endl;
    
    return true;
}

bool test_update_traversal_consistency() {
    print_test_header("Update Traversal Consistency Test");
    
    SBT_tree tree;
    
    // Insert multiple records
    std::vector<std::string> data = {"Apple", "Banana", "Cherry", "Date", "Elderberry"};
    
    for (const auto& item : data) {
        tree.insert((uchar*)item.c_str(), item.length());
    }
    
    std::cout << "Initial tree size: " << tree.get_count() << std::endl;
    
    // Update one record
    std::string new_value = "Blueberry";
    int result = tree.update((uchar*)data[1].c_str(), data[1].length(),
                            (uchar*)new_value.c_str(), new_value.length());
    
    if (result != SBT_SUCCESS) {
        std::cout << "Failed to update record" << std::endl;
        return false;
    }
    
    // Verify traversal still works
    std::cout << "Tree contents after update:" << std::endl;
    SBT_node *current = tree.get_first();
    int count = 0;
    
    while (current && count < 10) { // Safety limit
        std::string content((char*)current->data, current->length);
        std::cout << "  " << content << " (ID: " << current->insert_id << ")" << std::endl;
        current = tree.get_next(current);
        count++;
    }
    
    if (static_cast<uint>(count) != tree.get_count()) {
        std::cout << "Traversal count mismatch: " << count << " vs " << tree.get_count() << std::endl;
        return false;
    }
    
    // Verify updated record exists
    SBT_node *found = tree.find_by_data((uchar*)new_value.c_str(), new_value.length());
    if (!found) {
        std::cout << "Updated record not found in traversal" << std::endl;
        return false;
    }
    
    return true;
}

bool test_multiple_updates() {
    print_test_header("Multiple Updates Test");
    
    SBT_tree tree;
    
    // Insert initial data
    std::vector<std::string> initial_data = {"Record1", "Record2", "Record3", "Record4"};
    
    for (const auto& item : initial_data) {
        tree.insert((uchar*)item.c_str(), item.length());
    }
    
    std::cout << "Initial tree size: " << tree.get_count() << std::endl;
    
    // Perform multiple updates
    std::vector<std::string> updated_data = {"Updated1", "Updated2", "Updated3", "Updated4"};
    
    for (size_t i = 0; i < initial_data.size(); i++) {
        int result = tree.update((uchar*)initial_data[i].c_str(), initial_data[i].length(),
                                (uchar*)updated_data[i].c_str(), updated_data[i].length());
        
        if (result != SBT_SUCCESS) {
            std::cout << "Failed to update record " << i << std::endl;
            return false;
        }
    }
    
    std::cout << "Tree size after all updates: " << tree.get_count() << std::endl;
    
    // Verify all updates
    for (const auto& item : updated_data) {
        SBT_node *found = tree.find_by_data((uchar*)item.c_str(), item.length());
        if (!found) {
            std::cout << "Updated record not found: " << item << std::endl;
            return false;
        }
    }
    
    // Verify old records are gone
    for (const auto& item : initial_data) {
        SBT_node *found = tree.find_by_data((uchar*)item.c_str(), item.length());
        if (found) {
            std::cout << "Old record still exists: " << item << std::endl;
            return false;
        }
    }
    
    return true;
}

// Main test runner
int main() {
    std::cout << "SBT Tree Update Operations - Standalone Test" << std::endl;
    std::cout << "=============================================" << std::endl;
    
    bool all_passed = true;
    
    // Run all tests
    all_passed &= test_basic_update();
    all_passed &= test_update_different_length();
    all_passed &= test_update_nonexistent();
    all_passed &= test_update_invalid_arguments();
    all_passed &= test_update_traversal_consistency();
    all_passed &= test_multiple_updates();
    
    // Final results
    std::cout << "\n=============================================" << std::endl;
    if (all_passed) {
        std::cout << "🎉 ALL UPDATE TESTS PASSED! 🎉" << std::endl;
        std::cout << "SBT tree update operations are working correctly." << std::endl;
        return 0;
    } else {
        std::cout << "❌ SOME TESTS FAILED ❌" << std::endl;
        std::cout << "Please review the failed tests above." << std::endl;
        return 1;
    }
}