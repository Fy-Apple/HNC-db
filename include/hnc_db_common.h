#pragma once
#include <string>

namespace hnc::db::constant{
// TODO : 后续将所有参数 通过配置文件去读取

// 数据库id
constexpr std::string MYSQL_IP = "127.0.0.1";
constexpr int MYSQL_PORT = 3306;
// 端口号
constexpr std::string MYSQL_USER_NAME = "apple"; // 用户名
constexpr std::string MYSQL_PASSWORD = "201209"; // 密码
constexpr std::string MYSQL_DATABASE = "minecraft"; // 数据库名

// FOR TEST
constexpr int INIT_CONN_SIZE = 4; // 初始连接数
constexpr int THRESH_HOLD_CONN_SIZE = 8; // 最大连接数
constexpr int POOL_MAX_IDLE_TIME = 60; // 连接最大空闲时间， 每隔60秒检查一下连接池中是否有连接超过这个IDLE_time, 有则回收连接
constexpr int GET_CONN_TIMEOUT = 10; // 尝试从连接池中获取连接的超时时间
}