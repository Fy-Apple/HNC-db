#include "hnc_db_connection.h"


namespace hnc::db::details {

HncDBConnection::HncDBConnection() : m_alive_time_(clock()) {
    m_connection_ = std::make_shared<mysqlpp::Connection>();
}



HncDBConnection::~HncDBConnection() {
    if (m_is_connected()) {
        m_connection_->disconnect();
        std::cout << "disconnect\n";
    }
}

/**
 * @brief 连接mysql 数据库
 * @param server mysql服务器名 -> 127.0.0.1:3306
 * @param username 用户名
 * @param password 密码
 * @param dbname 数据库名
 * @return 返回连接状态
 */
bool HncDBConnection::connect(const std::string &server, const std::string &username, const std::string &password, const std::string &dbname) const noexcept{
    if (m_connection_->connect(dbname.c_str(), server.c_str(), username.c_str(), password.c_str())) {
        return true;
    }
    return false;
}

bool HncDBConnection::disconnect() const noexcept {
    if (m_is_connected()) {
        m_connection_->disconnect();
        return true;
    }
    return false;
}

/**
 * @brief insert, delete, update 语句
 */
bool HncDBConnection::update(const std::string &sql) const noexcept{
    if (!m_is_connected()) {
        return false;
    }

    mysqlpp::Query query = m_connection_->query(sql);
    return query.exec();
}

/**
 * @brief select 语句
 */
std::optional<mysqlpp::StoreQueryResult> HncDBConnection::query(const std::string &sql) const noexcept {
    if (!m_is_connected()) {
        return std::nullopt;
    }

    return m_connection_->query(sql).store();
}

/**
 * @brief 刷新该连接的存活时间, 用于后续回收使用
 */
void HncDBConnection::refresh_alive_time() noexcept {
    m_alive_time_ = clock();
}

/**
 * @brief 连接是否打开
 */
bool HncDBConnection::m_is_connected() const noexcept {
    return m_connection_ && m_connection_->connected();
}
}
