#ifndef SBT_TREE_H
#define SBT_TREE_H

#include "my_inttypes.h"
#include "sql/table.h"
#include <stack>
#include <atomic>

// Forward declarations
class SBT_cursor;

/**
 * SBT Node Structure
 * 
 * Represents a node in the Size Balanced Tree containing record data,
 * size information, and pointers to child subtrees.
 */
struct SBT_node {
    uchar *record_data;         // Record data
    size_t record_length;       // Record length
    uint64_t position;          // Record position identifier
    size_t size;                // Subtree size
    SBT_node *left;             // Left subtree
    SBT_node *right;            // Right subtree
    
    SBT_node(const uchar *data, size_t len, uint64_t pos);
    ~SBT_node();
    
    // Helper methods
    void update_size();
    size_t get_left_size() const { return left ? left->size : 0; }
    size_t get_right_size() const { return right ? right->size : 0; }
};

/**
 * SBT Tree Class
 * 
 * Implements the Size Balanced Tree data structure with basic operations
 * for insertion, deletion, search, and traversal.
 */
class SBT_tree {
private:
    SBT_node *root;             // Root node
    std::atomic<uint64_t> next_position; // Next position identifier
    std::atomic<size_t> total_records;   // Total record count
    TABLE *table_def;           // Table definition
    
    // Balance operations
    SBT_node* left_rotate(SBT_node *node);
    SBT_node* right_rotate(SBT_node *node);
    SBT_node* maintain(SBT_node *node, bool flag);
    void update_size(SBT_node *node);
    
    // Internal operations
    SBT_node* insert_node(SBT_node *root, const uchar *data, size_t len);
    SBT_node* delete_node(SBT_node *root, uint64_t position);
    SBT_node* find_node(SBT_node *root, uint64_t position);
    SBT_node* find_min_node(SBT_node *node);
    SBT_node* find_max_node(SBT_node *node);
    
    // Tree cleanup
    void destroy_tree(SBT_node *node);
    
public:
    SBT_tree(TABLE *table);
    ~SBT_tree();
    
    // Public interface
    int insert_record(const uchar *record, size_t length, uint64_t &position);
    int delete_record(uint64_t position);
    int update_record(uint64_t position, const uchar *new_data, size_t length);
    int find_record(uint64_t position, uchar *buffer, size_t &length);
    
    // Traversal support
    SBT_cursor* create_cursor();
    void destroy_cursor(SBT_cursor *cursor);
    
    // Statistics
    size_t get_record_count() const { return total_records.load(); }
    void get_statistics(ha_statistics &stats);
    
    // Tree validation (for testing)
    bool validate_tree() const;
    bool validate_node(SBT_node *node) const;
    
    // Access to root for cursor operations
    SBT_node* get_root() const { return root; }
};

/**
 * SBT Cursor Class
 * 
 * Provides in-order traversal of the SBT tree using a stack-based approach.
 */
class SBT_cursor {
private:
    std::stack<SBT_node*> node_stack;  // Node stack for in-order traversal
    SBT_node *current_node;            // Current node
    bool initialized;                  // Whether cursor is initialized
    
    void push_left_path(SBT_node *node);
    
public:
    SBT_cursor();
    ~SBT_cursor();
    
    void init(SBT_node *root);
    int next_record(uchar *buffer, size_t &length, uint64_t &position);
    void reset();
    bool is_valid() const;
    bool has_next() const;
};

#endif // SBT_TREE_H