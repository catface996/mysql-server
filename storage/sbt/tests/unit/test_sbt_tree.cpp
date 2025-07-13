#include <gtest/gtest.h>
#include "sbt_tree.h"

using namespace sbt;

class SBTTreeTest : public ::testing::Test {
protected:
    void SetUp() override {
        tree = std::make_unique<SBTTree<std::string, std::string>>();
    }
    
    void TearDown() override {
        tree.reset();
    }
    
    std::unique_ptr<SBTTree<std::string, std::string>> tree;
};

TEST_F(SBTTreeTest, BasicInsertAndFind) {
    // 测试基本插入和查找
    EXPECT_EQ(tree->insert("key1", "value1"), SBTError::SUCCESS);
    EXPECT_EQ(tree->insert("key2", "value2"), SBTError::SUCCESS);
    EXPECT_EQ(tree->insert("key3", "value3"), SBTError::SUCCESS);
    
    std::string value;
    EXPECT_EQ(tree->find("key1", value), SBTError::SUCCESS);
    EXPECT_EQ(value, "value1");
    
    EXPECT_EQ(tree->find("key2", value), SBTError::SUCCESS);
    EXPECT_EQ(value, "value2");
    
    EXPECT_EQ(tree->find("nonexistent", value), SBTError::KEY_NOT_FOUND);
}

TEST_F(SBTTreeTest, DuplicateKeyInsert) {
    // 测试重复键插入
    EXPECT_EQ(tree->insert("key1", "value1"), SBTError::SUCCESS);
    EXPECT_EQ(tree->insert("key1", "value2"), SBTError::DUPLICATE_KEY);
}

TEST_F(SBTTreeTest, UpdateOperation) {
    // 测试更新操作
    EXPECT_EQ(tree->insert("key1", "value1"), SBTError::SUCCESS);
    EXPECT_EQ(tree->update("key1", "new_value"), SBTError::SUCCESS);
    
    std::string value;
    EXPECT_EQ(tree->find("key1", value), SBTError::SUCCESS);
    EXPECT_EQ(value, "new_value");
    
    EXPECT_EQ(tree->update("nonexistent", "value"), SBTError::KEY_NOT_FOUND);
}

TEST_F(SBTTreeTest, DeleteOperation) {
    // 测试删除操作
    EXPECT_EQ(tree->insert("key1", "value1"), SBTError::SUCCESS);
    EXPECT_EQ(tree->insert("key2", "value2"), SBTError::SUCCESS);
    
    EXPECT_EQ(tree->remove("key1"), SBTError::SUCCESS);
    
    std::string value;
    EXPECT_EQ(tree->find("key1", value), SBTError::KEY_NOT_FOUND);
    EXPECT_EQ(tree->find("key2", value), SBTError::SUCCESS);
    
    EXPECT_EQ(tree->remove("nonexistent"), SBTError::KEY_NOT_FOUND);
}

TEST_F(SBTTreeTest, SizeAndEmpty) {
    // 测试大小和空状态
    EXPECT_TRUE(tree->empty());
    EXPECT_EQ(tree->size(), 0);
    
    tree->insert("key1", "value1");
    EXPECT_FALSE(tree->empty());
    EXPECT_EQ(tree->size(), 1);
    
    tree->insert("key2", "value2");
    EXPECT_EQ(tree->size(), 2);
    
    tree->remove("key1");
    EXPECT_EQ(tree->size(), 1);
    
    tree->remove("key2");
    EXPECT_TRUE(tree->empty());
    EXPECT_EQ(tree->size(), 0);
}

TEST_F(SBTTreeTest, RangeQuery) {
    // 测试范围查询
    tree->insert("apple", "fruit1");
    tree->insert("banana", "fruit2");
    tree->insert("cherry", "fruit3");
    tree->insert("date", "fruit4");
    tree->insert("elderberry", "fruit5");
    
    auto result = tree->range_query("banana", "date");
    EXPECT_EQ(result.size(), 3); // banana, cherry, date
    
    EXPECT_EQ(result[0].first, "banana");
    EXPECT_EQ(result[1].first, "cherry");
    EXPECT_EQ(result[2].first, "date");
}

TEST_F(SBTTreeTest, Iterator) {
    // 测试迭代器
    tree->insert("c", "3");
    tree->insert("a", "1");
    tree->insert("b", "2");
    tree->insert("d", "4");
    
    auto it = tree->begin();
    std::vector<std::string> keys;
    
    while (it.has_next()) {
        auto pair = it.next();
        keys.push_back(pair.first);
    }
    
    // 应该按字典序排列
    EXPECT_EQ(keys.size(), 4);
    EXPECT_EQ(keys[0], "a");
    EXPECT_EQ(keys[1], "b");
    EXPECT_EQ(keys[2], "c");
    EXPECT_EQ(keys[3], "d");
}

TEST_F(SBTTreeTest, LargeDataSet) {
    // 测试大数据集
    const int N = 10000;
    
    // 插入大量数据
    for (int i = 0; i < N; ++i) {
        std::string key = "key" + std::to_string(i);
        std::string value = "value" + std::to_string(i);
        EXPECT_EQ(tree->insert(key, value), SBTError::SUCCESS);
    }
    
    EXPECT_EQ(tree->size(), N);
    
    // 验证所有数据都能找到
    for (int i = 0; i < N; ++i) {
        std::string key = "key" + std::to_string(i);
        std::string expected_value = "value" + std::to_string(i);
        std::string actual_value;
        
        EXPECT_EQ(tree->find(key, actual_value), SBTError::SUCCESS);
        EXPECT_EQ(actual_value, expected_value);
    }
    
    // 验证SBT性质
    EXPECT_TRUE(tree->validate());
}

TEST_F(SBTTreeTest, SBTPropertyValidation) {
    // 测试SBT性质验证
    tree->insert("m", "13");
    tree->insert("f", "6");
    tree->insert("t", "20");
    tree->insert("c", "3");
    tree->insert("h", "8");
    tree->insert("p", "16");
    tree->insert("w", "23");
    
    // 验证SBT性质
    EXPECT_TRUE(tree->validate());
    
    // 删除一些节点后仍应保持SBT性质
    tree->remove("f");
    tree->remove("t");
    EXPECT_TRUE(tree->validate());
}

// 主函数
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
