#pragma once


#include "hnc_db_connection_pool.h"

#include "hnc_db_connection.h"
#include <hnc_db_common.h>

namespace hnc::db {

/**
 * @brief 全局创建连接池 的 API， 连接池也可以存在多个
 */
inline std::shared_ptr<details::HncDBConnectionPool> get_db_connection_pool(const int init_conn_size) {
    return std::make_shared<details::HncDBConnectionPool>(std::max(init_conn_size, constant::INIT_CONN_SIZE));
}

/**
 * @brief 全局 创建一个连接的 API
 */
inline std::shared_ptr<details::HncDBConnection> get_db_connection() {
    return std::make_shared<details::HncDBConnection>();
}

}
