#include "server.hpp"
#include <gtest/gtest.h>

class server_test : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        m_server = ns::make_tcp_server();

        // Setup connection bindings
        m_server
            ->on_server_error([](auto server, const auto &ec) {
                loge("Server error: ", ec.message());
            })
            ->on_read([](auto client, auto msg) { logi("Read data"); })
            ->on_close([](auto client) { logi("Connection closed"); })
            ->on_error([](auto client, const auto &ec) {
                loge("Error: ", ec.message());
            })
            ->on_accept([](auto client) {
                logi("Connection accepted");
                client->run();
            });
    }

    std::shared_ptr<ns::tcp_server> m_server;
};

TEST_F(server_test, server_accepts_client)
{
}
