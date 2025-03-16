#include "hnc_db_connection_pool.h"
#include "hnc_db_connection.h"
#include "hnc_db_common.h"

#include <thread>

#include <chrono>


namespace hnc::db::details {

HncDBConnectionPool::HncDBConnectionPool(const size_t init_conn_size = constant::INIT_CONN_SIZE)
    : m_init_conn_size_(init_conn_size), m_real_conn_count_(0), m_task_fd_(-1), m_running_(false){
    // 初始化连接参数
    m_init();
}

HncDBConnectionPool::~HncDBConnectionPool() {
    {
        std::unique_lock<std::mutex> locker(m_queue_mtx_);
        // 通知定时器 结束循环任务
        m_timer_->remove_timer(m_task_fd_);

        // 唤醒后台线程退出
        m_running_ = false;
        m_cond_.notify_all();
    }
    // 等待后台线程退出
    m_lch_.wait();

    // 删除连接资源
    while (!m_conn_que_.empty()) {
        const auto conn = m_conn_que_.front();
        m_conn_que_.pop_front();
        delete conn;
    }
}

/**
 * @brief 启动该连接池， 读取 配置信息 并连接数据库
 */
void HncDBConnectionPool::start() noexcept {
    // 创建 初始数量的 连接
    for (int i = 0; i < m_init_conn_size_; ++i) {
        m_conn_que_.emplace_back(new HncDBConnection()); // new出来的资源后续会被后台回收线程 delete掉, 以及析构函数delete掉
        if (m_conn_que_.back()->connect(m_server_, m_username_, m_password_, m_dbname_)){
            m_conn_que_.back()->refresh_alive_time(); // 初始化连接的 存活时间
            ++m_real_conn_count_; // 这里不需要设置内存序， 因为初始化创建连接 没有线程竞争
        }
        else {
            m_conn_que_.pop_back();
            std::cerr << "HncDBConnectionPool create initial connection->"<< i + 1 <<" failed!\n";
        }
    }
    // 启动后台线程
    m_running_ = true;

    // 启动一个后台线程 负责 在连接不足时创建新的连接
    std::thread creator(m_thread_func_create_conn, this);
    creator.detach();

    // 启动一个 定时后台线程 负责 回收 超过空闲时间的 超过初始化线程数量的连接
    // TODO : 定时器直接复用 core插件的定时器， 定时器需要额外在实现一个 线程数 为 1 时 不去启动额外的线程的功能
    m_timer_ = core::timer::details::get_timer_manager(2);

    m_task_fd_ = m_timer_->add_timer(static_cast<std::chrono::seconds>(m_max_idle_time_),[this]()->void {
        this->m_thread_func_recycle_conn();
    }, true);


    // TODO : 是否需要等待后台线程创建完毕 ？ 是否需要使用线程池来维护 ？
}

/**
 * @brief 返回一个智能指针管理的 连接， 该智能指针析构时 会把连接还给连接池的队列
 */
std::shared_ptr<HncDBConnection> HncDBConnectionPool::get_connection() noexcept {
    std::unique_lock<std::mutex> lock(m_queue_mtx_);
    // 检查队列是否为空
    while (m_conn_que_.empty())
    {
        // 等待从连接池中获取连接， 若超时则返回 nullptr,
        if (std::cv_status::timeout == m_cond_.wait_for(lock, std::chrono::seconds(m_conn_timeout_)))
        {
            // 检查一下是否是超时醒来， 则直接返回空
            if (m_conn_que_.empty())
            {
                std::cout << "get connection timeout ! return nullptr conn!\n";
                return nullptr;
            }
        }
    }

    /**
     *  自定义智能指针删除器， 从用户空间析构时， 归还到连接池的队列中
     */
    std::shared_ptr<HncDBConnection> conn_ptr_(m_conn_que_.front(),
        [&](HncDBConnection *conn_p) {
        // 该 删除器 在 用户空间调用， 需要和连接池互锁
        std::unique_lock<std::mutex> locker(m_queue_mtx_);
        conn_p->refresh_alive_time(); // 刷新空闲的起始时间
        m_conn_que_.push_back(conn_p);
    });

    // 使用一个连接后， 尝试通知后台线程创建新的连接
    m_conn_que_.pop_front();
    m_cond_.notify_all();  // 消费完连接以后，通知生产者线程检查一下，如果队列为空了，赶紧生产连接

    return conn_ptr_;
}


/**
 * @brief 初始化连接池参数，
 * TODO : 后续从 全局配置文件中读取
 */
void HncDBConnectionPool::m_init() noexcept {
    m_server_ = constant::MYSQL_IP + ":" + std::to_string(constant::MYSQL_PORT);
    m_username_ = constant::MYSQL_USER_NAME;
    m_password_ = constant::MYSQL_PASSWORD;
    m_dbname_ = constant::MYSQL_DATABASE;
    m_thresh_hold_conn_size_ = std::max(m_init_conn_size_, constant::THRESH_HOLD_CONN_SIZE); // 最大连接数必须大于初始连接数
    m_max_idle_time_ = constant::POOL_MAX_IDLE_TIME;
    m_conn_timeout_ = constant::GET_CONN_TIMEOUT;
}

/**
 * @brief 连接池后台线程，在连接为空时 创建 新的连接， 但是连接最大不能超过m_thresh_hold_conn_size_
 */
void HncDBConnectionPool::m_thread_func_create_conn() noexcept {
    while (true) {
        std::unique_lock<std::mutex> lock(m_queue_mtx_);

        if (!m_running_)  break;
        while (!m_conn_que_.empty())
        {
            // 连接不为空时 等待在 条件变量上， 等待连接为空后被唤醒
            m_cond_.wait(lock);
        }

        if (!m_running_) break;

        // 连接数量没有到达上限，继续创建新的连接
        if (m_real_conn_count_.load(std::memory_order_acquire) < m_thresh_hold_conn_size_)
        {
            m_conn_que_.emplace_back(new HncDBConnection());
            if (m_conn_que_.back()->connect(m_server_, m_username_, m_password_, m_dbname_)){
                m_conn_que_.back()->refresh_alive_time(); // 初始化连接的 存活时间
                ++m_real_conn_count_; // 这里不需要设置内存序， 因为初始化创建连接 没有线程竞争
            } else {
                // 否则创建失败 则将新建的连接去掉
                auto conn = m_conn_que_.back();
                m_conn_que_.pop_back();
                delete conn;
                std::cerr << "HncDB CreateThread create connection failed!\n";
            }
        }

        // 通知消费者线程，已经有新的连接可以使用了
        m_cond_.notify_all();
    }

    // 两个后台线程都退出时就可以通知主线程析构了
    m_lch_.count_down();
}

/**
 * @brief 定时器线程负责定时执行的 任务， 扫描超过 一定空闲时间的连接， 回收连接
 */
void HncDBConnectionPool::m_thread_func_recycle_conn() noexcept {
    // 扫描整个队列，释放多余的连接
    std::unique_lock<std::mutex> lock(m_queue_mtx_);
    // 当前连接数大于初始连接数，则将 空闲时间超过一定idle time 的连接回收
    while (m_real_conn_count_ > m_init_conn_size_)
    {
        if (m_conn_que_.front()->get_alive_time() >= (m_max_idle_time_ * 1000))
        {
            const auto conn = m_conn_que_.front();
            m_conn_que_.pop_front();
            --m_real_conn_count_;
            delete conn; //  显式调用delete 调用连接的析构函数释放连接资源
        }
        else
        {
            break; // 队头的连接没有超过_maxIdleTime，其它连接肯定没有
        }
    }

}


}
