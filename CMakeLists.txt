# 设置头文件搜索路径
include_directories(include)


# 设置源文件
set(SOURCES
        src/hnc_db_connection.cpp
        src/hnc_db_connection_pool.cpp
)


# 生成静态库
add_library(hnc_db STATIC ${SOURCES})

target_link_libraries(hnc_db hnc_core)


# test
add_subdirectory(test)
