#pragma once

#include <optional>
#include <string>
#include <time.h>

#include <mysql++/mysql++.h>

namespace hnc::db::details {

/**
 * @brief
 */
class HncDBConnection {
public:
    HncDBConnection();
    ~HncDBConnection();


    /**
     * @brief 连接mysql 数据库
     * @param server mysql服务器名 -> 127.0.0.1:3306
     * @param username 用户名
     * @param password 密码
     * @param dbname 数据库名
     * @return 返回连接状态
     */
    bool connect(const std::string &server, const std::string &username, const std::string &password, const std::string &dbname) const noexcept;

    bool disconnect() const noexcept;

    /**
     * @brief insert, delete, update 语句
     */
    bool update(const std::string &sql) const noexcept;

    /**
     * @brief select 语句
     */
    std::optional<mysqlpp::StoreQueryResult> query(const std::string &sql) const noexcept;

    /**
     * @brief 刷新该连接的存活时间, 用于后续回收使用
     */
    void refresh_alive_time() noexcept ;

    /**
     * @brief 获取当前连接的空闲时间
     */
    clock_t get_alive_time() const noexcept { return clock() - m_alive_time_; }

private:

    /**
     * @brief 连接是否打开
     */
    bool m_is_connected() const noexcept;
    std::shared_ptr<mysqlpp::Connection> m_connection_; // 长连接
    clock_t m_alive_time_; // 最近一次使用后的存活时间
};



}