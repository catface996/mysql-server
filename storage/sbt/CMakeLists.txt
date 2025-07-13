# SBT Storage Engine CMakeLists.txt (Simplified Version)
cmake_minimum_required(VERSION 3.16)

# 设置项目信息
set(STORAGE_ENGINE_NAME "sbt")
set(STORAGE_ENGINE_VERSION "1.0.0")

# 包含MySQL的头文件
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/sql
    ${CMAKE_SOURCE_DIR}/storage
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 设置源文件（简化版本）
set(SBT_SOURCES
    # 核心SBT算法
    src/core/sbt_node.cpp
    src/core/sbt_tree.cpp
    
    # MySQL接口
    src/mysql_interface/ha_sbt.cpp
)

# 设置头文件
set(SBT_HEADERS
    include/sbt_common.h
    include/sbt_node.h
    include/sbt_tree.h
    include/ha_sbt.h
)

# 创建存储引擎库
mysql_add_plugin(${STORAGE_ENGINE_NAME} 
    ${SBT_SOURCES} 
    STORAGE_ENGINE
    MODULE_ONLY
    LINK_LIBRARIES ${CMAKE_THREAD_LIBS_INIT}
)

# 设置编译选项
target_compile_features(${STORAGE_ENGINE_NAME} PRIVATE cxx_std_17)
target_compile_options(${STORAGE_ENGINE_NAME} PRIVATE
    $<$<CONFIG:Debug>:-g -O0 -DDEBUG -DSAFEMALLOC -DSAFE_MUTEX>
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
)

# 设置编译定义
target_compile_definitions(${STORAGE_ENGINE_NAME} PRIVATE
    MYSQL_DYNAMIC_PLUGIN
)

# 安装头文件
install(FILES ${SBT_HEADERS} DESTINATION include/mysql/storage/sbt)
