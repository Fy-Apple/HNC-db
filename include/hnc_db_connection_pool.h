#pragma once

#include "hnc_timer.h"

#include <queue>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <latch>

namespace hnc::db::details {

class HncDBConnection;

class HncDBConnectionPool {
public:
    // 由全局接口 创建连接池的 智能指针实例
    explicit HncDBConnectionPool(size_t init_conn_size);
    ~HncDBConnectionPool();

    /**
     * @brief 启动该连接池， 读取 配置信息 并连接数据库
     */
    void start() noexcept ;


    /**
     * @brief 返回一个智能指针管理的 连接， 该智能指针析构时 会把连接还给连接池的队列
     */
    std::shared_ptr<HncDBConnection> get_connection() noexcept;


private:

    /**
     * @brief 初始化连接池参数，
     * TODO : 后续从 全局配置文件中读取
     */
    void m_init() noexcept;


    /**
     * @brief 连接池后台线程， 负责创建新的连接
     */
    void m_thread_func_create_conn() noexcept;

    /**
     * @brief 扫描超过 一定空闲时间的连接， 回收连接
     */
    void m_thread_func_recycle_conn() noexcept;

    // 删除拷贝 和 赋值 移动
    HncDBConnectionPool(HncDBConnectionPool &) = delete;
    HncDBConnectionPool(const HncDBConnectionPool &&) = delete;
    HncDBConnectionPool& operator=(const HncDBConnectionPool &) = delete;
    HncDBConnectionPool& operator=(HncDBConnectionPool &&) = delete;

private:
    std::string m_server_; // 服务器ip+port
    std::string m_username_; // 用户名
    std::string m_password_; // 密码
    std::string m_dbname_; // 数据库名称
    int m_init_conn_size_; // 连接池的初始连接量
    int m_thresh_hold_conn_size_; // 连接池的最大连接量
    int m_max_idle_time_; // 连接池最大空闲时间, 超时后负责回收连接
    int m_conn_timeout_; // 连接池获取连接的超时时间


    std::deque<HncDBConnection*> m_conn_que_; // 连接队列
    std::mutex m_queue_mtx_; // 队列互斥锁
    std::atomic_int m_real_conn_count_; // 记录连接所创建的connection连接的总数量
    std::condition_variable m_cond_; // 连接生产线程和连接消费线程的通信

    // 负责管理后台定时任务
    std::shared_ptr<core::timer::details::HncTimerManager> m_timer_;
    int m_task_fd_;

    // 负责等待后台线程退出
    std::latch m_lch_{1};
    bool m_running_;
};


}