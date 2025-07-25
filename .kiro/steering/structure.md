# 项目结构与组织

## 顶级目录布局

### 核心服务器组件
- **`sql/`**: MySQL 服务器主要代码（SQL 解析器、优化器、执行器）
- **`storage/`**: 存储引擎实现（InnoDB、MyISAM、NDB 等）
  - **`storage/innobase/`**: InnoDB 存储引擎完整实现
- **`mysys/`**: MySQL 系统库（跨平台工具）
- **`include/`**: 公共头文件和 API
- **`strings/`**: 字符串操作和字符集处理

### 客户端与工具
- **`client/`**: MySQL 客户端程序（mysql、mysqldump、mysqladmin 等）
- **`libmysql/`**: MySQL 客户端库
- **`utilities/`**: 各种实用程序

### 库与组件
- **`components/`**: MySQL 组件系统（插件、服务）
- **`plugin/`**: 服务器插件（认证、存储引擎等）
- **`libservices/`**: 插件 API 的服务实现
- **`libs/`**: 附加库和脚本

### 构建与配置
- **`cmake/`**: CMake 构建配置文件
- **`extra/`**: 第三方库（捆绑依赖）
- **`packaging/`**: 发行版打包脚本

### 测试与文档
- **`mysql-test/`**: MySQL 测试套件（MTR）
- **`unittest/`**: 单元测试
- **`testclients/`**: 测试客户端程序
- **`man/`**: 手册页
- **`Docs/`**: 文档文件

### 复制与集群
- **`libbinlogevents/`**: 二进制日志事件处理
- **`libchangestreams/`**: 变更流功能
- **`router/`**: MySQL 路由器（如果存在）

## 关键 SQL 目录结构

`sql/` 目录包含按功能组织的核心服务器逻辑：

### 核心引擎
- **解析器**: `sql_yacc.yy`、`sql_lex.cc`、`lex.h`
- **优化器**: `sql_optimizer.cc`、`opt_*` 文件、`join_optimizer/`
- **执行器**: `sql_executor.cc`、`iterators/`
- **处理器接口**: `handler.cc`、`handler.h`

### SQL 操作
- **DML**: `sql_select.cc`、`sql_insert.cc`、`sql_update.cc`、`sql_delete.cc`
- **DDL**: `sql_table.cc`、`sql_alter.cc`、`dd/`（数据字典）
- **事务**: `transaction.cc`、`xa/`

### 专门领域
- **复制**: `rpl_*` 文件、`binlog/`
- **分区**: `partitioning/`、`sql_partition.cc`
- **安全**: `auth/`、`sql_connect.cc`
- **GIS**: `gis/`、`item_geofunc.cc`

## 架构模式

### 分层架构
1. **SQL 层**: 解析、优化、执行
2. **存储引擎层**: 可插拔存储后端
3. **系统层**: 操作系统抽象、内存管理

### 插件架构
- 存储引擎作为插件
- 认证插件
- 基于组件的服务
- UDF（用户定义函数）

### 处理器模式
- 存储引擎接口的抽象 `handler` 类
- 每个存储引擎实现处理器方法
- 不同存储后端间的一致 API

## 文件命名约定

### 源文件
- **`.cc`**: C++ 实现文件
- **`.h`**: 头文件
- **`.yy`**: Bison 解析器文件
- **`.ll`**: Flex 词法分析器文件

### 前缀
- **`sql_`**: 核心 SQL 功能
- **`rpl_`**: 复制相关代码
- **`my_`**: MySQL 系统工具
- **`item_`**: SQL 表达式项
- **`opt_`**: 优化器组件

#### InnoDB 特定前缀
- **`ha_`**: Handler 接口实现
- **`buf0`**: 缓冲池相关
- **`btr0`**: B+树相关
- **`dict0`**: 数据字典相关
- **`fil0`**: 文件管理相关
- **`fsp0`**: 文件空间相关
- **`lock0`**: 锁管理相关
- **`log0`**: 日志系统相关
- **`mtr0`**: Mini-transaction 相关
- **`page0`**: 页面管理相关
- **`row0`**: 行操作相关
- **`srv0`**: 服务器相关
- **`sync0`**: 同步相关
- **`trx0`**: 事务相关
- **`ut0`**: 工具函数相关

### 测试文件
- **`*_test.cc`**: 单元测试
- **`*.test`**: MTR 测试用例
- **`*.result`**: 预期测试结果

## InnoDB 存储引擎详细结构

### `storage/innobase/` 目录组织

#### 核心子系统
- **`handler/`**: MySQL 接口层
  - `ha_innodb.cc/h`: 主要的存储引擎接口实现
  - `ha_innopart.cc/h`: 分区表支持
  - `handler0alter.cc`: DDL 操作处理
  - `i_s.cc/h`: Information Schema 表
  - `p_s.cc/h`: Performance Schema 集成

#### 存储管理
- **`buf/`**: 缓冲池管理
  - `buf0buf.cc`: 缓冲池核心逻辑
  - `buf0lru.cc`: LRU 替换算法
  - `buf0flu.cc`: 脏页刷新
  - `buf0dblwr.cc`: 双写缓冲区
- **`fil/`**: 文件空间管理
- **`fsp/`**: 文件空间分配
- **`page/`**: 页面管理和压缩

#### 索引和数据结构
- **`btr/`**: B+树索引实现
  - `btr0btr.cc`: B+树基本操作
  - `btr0cur.cc`: B+树游标
  - `btr0sea.cc`: 自适应哈希索引
- **`dict/`**: 数据字典管理
- **`row/`**: 行操作（插入、更新、删除、查询）
- **`rem/`**: 记录格式和比较

#### 事务系统
- **`trx/`**: 事务管理
  - `trx0trx.cc`: 事务对象
  - `trx0sys.cc`: 事务系统
  - `trx0undo.cc`: 撤销日志
  - `trx0purge.cc`: 清理线程
- **`lock/`**: 锁管理
  - `lock0lock.cc`: 锁系统核心
  - `lock0wait.cc`: 锁等待处理

#### 日志系统
- **`log/`**: 重做日志系统
  - `log0log.cc`: 日志核心功能
  - `log0recv.cc`: 崩溃恢复
  - `log0write.cc`: 日志写入
  - `log0chkp.cc`: 检查点

#### 并发控制
- **`sync/`**: 同步原语
- **`mtr/`**: Mini-transaction（最小事务单元）
- **`read/`**: 一致性读视图

#### 专门功能
- **`fts/`**: 全文搜索
- **`gis/`**: 地理信息系统支持
- **`lob/`**: 大对象（BLOB/TEXT）处理
- **`clone/`**: 克隆功能
- **`ddl/`**: 在线 DDL 操作

#### 系统服务
- **`srv/`**: 服务器主程序和监控
- **`os/`**: 操作系统抽象层
- **`ut/`**: 通用工具函数
- **`mem/`**: 内存管理

#### 头文件组织
- **`include/`**: 所有 InnoDB 内部头文件
  - 按模块组织，与源码目录对应
  - `.ic` 文件包含内联函数实现

### InnoDB 架构模式

#### 分层设计
1. **Handler 层**: 与 MySQL 服务器接口
2. **事务层**: 事务管理和 MVCC
3. **索引层**: B+树和哈希索引
4. **存储层**: 页面管理和文件 I/O
5. **系统层**: 内存、锁、日志管理

#### 关键设计模式
- **RAII**: 资源自动管理（锁、内存等）
- **观察者模式**: 事件通知系统
- **策略模式**: 不同的页面压缩算法
- **工厂模式**: 对象创建和初始化

## 构建产物
- **`build/`**: 源码外构建目录（推荐）
- **`cmake-build-debug/`**: IDE 生成的构建目录
- 生成的文件与源代码分离