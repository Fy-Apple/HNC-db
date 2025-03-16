#include <iostream>

#include "hnc_db.h"

#include "benchmark.h"
#include <thread>

using namespace std;

using namespace hnc::db;


/**
 * @brief 测试单挑连接是否可正常执行sql
 */
void test_connection() {
    //测试获取一个连接
    auto conn = get_db_connection();
    bool result = conn->connect("127.0.0.1:3306", "apple", "201209", "minecraft");

    if (!result) {
        std::cerr << "connect to mysql failed!\n";
        assert(false);
    }

    // 测试插入一条数据
    result = conn->update("INSERT INTO users(name) VALUES('mc_test')");

    if (result) {
        std::cerr << "insert to mysql successful!\n";
    }else {
        std::cerr << "insert to mysql failed!  sql = mc_test\n";

    }
}

/**
 * @brief 测试多线程环境下 连接池是否正常工作
 */
void test_connection_pool_mt() {

    auto pool = get_db_connection_pool(4);

    // 测试 连接池

    {
        TICK(test_pool_time)

        thread t1([&]() {
            for (int i = 0; i < 30; ++i)
            {
                const auto conn_s_ptr_ = pool->get_connection();
                char sql[1024] = { 0 };
                sprintf(sql, "INSERT INTO users(name) values('%s')","t1_" + i);
                conn_s_ptr_->update(sql);
            }
        });
        thread t2([&]() {
            for (int i = 0; i < 30; ++i)
            {
                const auto conn_s_ptr_ = pool->get_connection();
                char sql[1024] = { 0 };
                sprintf(sql, "INSERT INTO users(name) values('%s')","t2_" + i);
                conn_s_ptr_->update(sql);
            }
        });
        thread t3([&]() {
            for (int i = 0; i < 30; ++i)
            {
                const auto conn_s_ptr_ = pool->get_connection();
                char sql[1024] = { 0 };
                sprintf(sql, "INSERT INTO users(name) values('%s')","t3_" + i);
                conn_s_ptr_->update(sql);
            }
        });
        thread t4([&]() {
            for (int i = 0; i < 30; ++i)
            {
                const auto conn_s_ptr_ = pool->get_connection();
                char sql[1024] = { 0 };
                sprintf(sql, "INSERT INTO users(name) values('%s')","t4_" + i);
                conn_s_ptr_->update(sql);
            }
        });

        t1.join();
        t2.join();
        t3.join();
        t4.join();
        TOCK(test_pool_time)
    }
}


int main()
{
    test_connection();
    // test_connection_pool_mt();

	return 0;
}
