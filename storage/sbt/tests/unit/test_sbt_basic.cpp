#include <iostream>
#include <string>
#include <vector>
#include <cassert>

// 包含SBT头文件
#include "../../include/sbt_tree.h"

using namespace sbt;

void test_basic_operations() {
    std::cout << "Testing basic SBT operations..." << std::endl;
    
    SBTTree<std::string, std::vector<unsigned char>> tree;
    
    // 测试插入
    std::string key1 = "apple";
    std::vector<unsigned char> value1 = {'a', 'p', 'p', 'l', 'e'};
    assert(tree.insert(key1, value1) == SBTError::SUCCESS);
    
    std::string key2 = "banana";
    std::vector<unsigned char> value2 = {'b', 'a', 'n', 'a', 'n', 'a'};
    assert(tree.insert(key2, value2) == SBTError::SUCCESS);
    
    std::string key3 = "cherry";
    std::vector<unsigned char> value3 = {'c', 'h', 'e', 'r', 'r', 'y'};
    assert(tree.insert(key3, value3) == SBTError::SUCCESS);
    
    // 测试查找
    std::vector<unsigned char> found_value;
    assert(tree.find(key1, found_value) == SBTError::SUCCESS);
    assert(found_value == value1);
    
    assert(tree.find(key2, found_value) == SBTError::SUCCESS);
    assert(found_value == value2);
    
    // 测试不存在的键
    assert(tree.find("nonexistent", found_value) == SBTError::KEY_NOT_FOUND);
    
    // 测试重复插入
    assert(tree.insert(key1, value1) == SBTError::DUPLICATE_KEY);
    
    // 测试大小
    assert(tree.size() == 3);
    assert(!tree.empty());
    
    // 测试更新
    std::vector<unsigned char> new_value1 = {'A', 'P', 'P', 'L', 'E'};
    assert(tree.update(key1, new_value1) == SBTError::SUCCESS);
    assert(tree.find(key1, found_value) == SBTError::SUCCESS);
    assert(found_value == new_value1);
    
    // 测试删除
    assert(tree.remove(key2) == SBTError::SUCCESS);
    assert(tree.find(key2, found_value) == SBTError::KEY_NOT_FOUND);
    assert(tree.size() == 2);
    
    // 测试SBT性质验证
    assert(tree.validate());
    
    std::cout << "Basic operations test passed!" << std::endl;
}

void test_large_dataset() {
    std::cout << "Testing large dataset..." << std::endl;
    
    SBTTree<std::string, std::vector<unsigned char>> tree;
    
    const int N = 1000;
    
    // 插入大量数据
    for (int i = 0; i < N; ++i) {
        std::string key = "key" + std::to_string(i);
        std::string value_str = "value" + std::to_string(i);
        std::vector<unsigned char> value(value_str.begin(), value_str.end());
        
        assert(tree.insert(key, value) == SBTError::SUCCESS);
    }
    
    assert(tree.size() == N);
    
    // 验证所有数据
    for (int i = 0; i < N; ++i) {
        std::string key = "key" + std::to_string(i);
        std::string expected_value_str = "value" + std::to_string(i);
        std::vector<unsigned char> expected_value(expected_value_str.begin(), expected_value_str.end());
        
        std::vector<unsigned char> found_value;
        assert(tree.find(key, found_value) == SBTError::SUCCESS);
        assert(found_value == expected_value);
    }
    
    // 验证SBT性质
    assert(tree.validate());
    
    std::cout << "Large dataset test passed!" << std::endl;
}

void test_iterator() {
    std::cout << "Testing iterator..." << std::endl;
    
    SBTTree<std::string, std::vector<unsigned char>> tree;
    
    // 插入一些数据
    std::vector<std::string> keys = {"dog", "apple", "cat", "banana"};
    for (const auto& key : keys) {
        std::vector<unsigned char> value(key.begin(), key.end());
        tree.insert(key, value);
    }
    
    // 使用迭代器遍历（应该按字典序）
    auto it = tree.begin();
    std::vector<std::string> result_keys;
    
    while (it.has_next()) {
        auto pair = it.next();
        result_keys.push_back(pair.first);
    }
    
    // 验证顺序
    std::vector<std::string> expected = {"apple", "banana", "cat", "dog"};
    assert(result_keys == expected);
    
    std::cout << "Iterator test passed!" << std::endl;
}

void test_range_query() {
    std::cout << "Testing range query..." << std::endl;
    
    SBTTree<std::string, std::vector<unsigned char>> tree;
    
    // 插入数据
    std::vector<std::string> keys = {"apple", "banana", "cherry", "date", "elderberry", "fig"};
    for (const auto& key : keys) {
        std::vector<unsigned char> value(key.begin(), key.end());
        tree.insert(key, value);
    }
    
    // 范围查询
    auto result = tree.range_query("banana", "date");
    
    assert(result.size() == 3);
    assert(result[0].first == "banana");
    assert(result[1].first == "cherry");
    assert(result[2].first == "date");
    
    std::cout << "Range query test passed!" << std::endl;
}

int main() {
    std::cout << "Starting SBT tests..." << std::endl;
    
    try {
        test_basic_operations();
        test_large_dataset();
        test_iterator();
        test_range_query();
        
        std::cout << "All tests passed!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}
