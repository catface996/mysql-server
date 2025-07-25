# 需求文档

## 介绍

本文档概述了基于SBT（Size Balanced Tree）算法为MySQL实现简化存储引擎的需求。该存储引擎将提供基本的CRUD操作（创建、读取、更新、删除）以及数据持久化到磁盘的功能。这是一个最小化实现，专注于演示自定义存储引擎的核心概念，不包含主键、索引或事务支持等高级功能。

## 需求

### 需求 1

**用户故事：** 作为数据库管理员，我希望能够使用SBT存储引擎创建表，以便使用Size Balanced Tree数据结构存储数据。

#### 验收标准

1. WHEN CREATE TABLE语句指定ENGINE=SBT THEN 系统 SHALL 使用SBT存储引擎创建表
2. WHEN 表被创建 THEN 系统 SHALL 在内存中初始化SBT数据结构
3. WHEN 表被创建 THEN 系统 SHALL 在磁盘上创建相应的数据文件用于持久化

### 需求 2

**用户故事：** 作为数据库用户，我希望能够向SBT表中插入记录，以便持久化存储数据。

#### 验收标准

1. WHEN INSERT语句在SBT表上执行 THEN 系统 SHALL 将记录添加到SBT数据结构中
2. WHEN 记录被插入 THEN 系统 SHALL 维护SBT的平衡属性
3. WHEN 记录被插入 THEN 系统 SHALL 将更改持久化到磁盘
4. WHEN 插入多条记录 THEN 系统 SHALL 维护数据完整性

### 需求 3

**用户故事：** 作为数据库用户，我希望能够从SBT表中查询记录，以便检索存储的数据。

#### 验收标准

1. WHEN SELECT语句在SBT表上执行 THEN 系统 SHALL 遍历SBT结构查找匹配的记录
2. WHEN 找到记录 THEN 系统 SHALL 以一致的顺序返回它们
3. WHEN 没有记录匹配查询 THEN 系统 SHALL 返回空结果集
4. WHEN 表为空 THEN 系统 SHALL 优雅地处理查询

### 需求 4

**用户故事：** 作为数据库用户，我希望能够更新SBT表中的现有记录，以便修改存储的数据。

#### 验收标准

1. WHEN UPDATE语句在SBT表上执行 THEN 系统 SHALL 在SBT结构中定位目标记录
2. WHEN 记录被更新 THEN 系统 SHALL 在维护SBT属性的同时修改数据
3. WHEN 记录被更新 THEN 系统 SHALL 将更改持久化到磁盘
4. WHEN 没有记录匹配更新条件 THEN 系统 SHALL 无错误地完成

### 需求 5

**用户故事：** 作为数据库用户，我希望能够从SBT表中删除记录，以便移除不需要的数据。

#### 验收标准

1. WHEN DELETE语句在SBT表上执行 THEN 系统 SHALL 在SBT结构中定位并移除目标记录
2. WHEN 记录被删除 THEN 系统 SHALL 根据需要重新平衡SBT结构
3. WHEN 记录被删除 THEN 系统 SHALL 将更改持久化到磁盘
4. WHEN 没有记录匹配删除条件 THEN 系统 SHALL 无错误地完成

### 需求 6

**用户故事：** 作为数据库管理员，我希望SBT存储引擎能够将数据持久化到磁盘，以便数据在服务器重启后仍然存在。

#### 验收标准

1. WHEN 服务器启动 THEN 系统 SHALL 从磁盘文件加载现有的SBT表数据
2. WHEN 数据被修改 THEN 系统 SHALL 将更改写入磁盘文件
3. WHEN 服务器关闭 THEN 系统 SHALL 确保所有数据都正确保存到磁盘
4. WHEN 磁盘文件损坏 THEN 系统 SHALL 优雅地处理错误

### 需求 7

**用户故事：** 作为数据库管理员，我希望能够删除SBT表，以便移除表及其关联的数据文件。

#### 验收标准

1. WHEN DROP TABLE语句在SBT表上执行 THEN 系统 SHALL 从内存中移除表
2. WHEN 表被删除 THEN 系统 SHALL 从磁盘删除关联的数据文件
3. WHEN 表被删除 THEN 系统 SHALL 清理所有分配的资源
4. WHEN 尝试删除不存在的表 THEN 系统 SHALL 返回适当的错误

### 需求 8

**用户故事：** 作为数据库开发者，我希望SBT存储引擎能够与MySQL的处理器接口集成，以便与MySQL服务器无缝协作。

#### 验收标准

1. WHEN 存储引擎被加载 THEN 系统 SHALL 向MySQL的存储引擎框架注册
2. WHEN 执行SQL操作 THEN 系统 SHALL 正确实现MySQL处理器接口方法
3. WHEN 使用存储引擎 THEN 系统 SHALL 遵循MySQL的存储引擎约定和模式
4. WHEN 发生错误 THEN 系统 SHALL 返回适当的MySQL错误代码