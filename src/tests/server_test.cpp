#include "client.hpp"
#include "server.hpp"
#include <gtest/gtest.h>

class server_test : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        ns::cout.set_debug(true);

        m_server = ns::make_tcp_server(6666);

        // Setup connection bindings
        m_server->use_certificate("cert.crt", "cert.key")
            ->on_server_error([](auto server, const auto &ec) {
                loge("server error: ", ec.message());
            })
            ->on_read([](auto client, auto data) {
                EXPECT_EQ(data.type(), ns::data_type::auth);
                logi("read data type: ", data.type());
                client->close();
            })
            ->on_close([](auto client) { logi("connection closed"); })
            ->on_error([](auto client, const auto &ec) {
                loge("error: ", ec.message());
            })
            ->on_accept([](auto client) {
                logi("connection accepted");
                client->run();
            })
            ->start();
    }

    virtual void TearDown()
    {
        m_server->stop();
    }

    std::shared_ptr<ns::tcp_server<>> m_server;
};

TEST_F(server_test, server_accepts_client)
{
    // Connect to our m_server, and close the connection when we connect.
    // This will cause the run loop to stop and exit the function.
    auto client = ns::make_tcp_client();
    client
        ->on_connect([](auto c) {
            logi("client connected");
            c->close();
        })
        ->on_read([](auto client, auto data) { logi("read data"); })
        ->on_close([](auto c) { logi("client closed"); })
        ->on_error([](auto c, const auto &e) {})
        ->run("localhost", "6666");
}

TEST_F(server_test, server_serializes_properly)
{
    // Connect to our m_server, and close the connection when we connect.
    // This will cause the run loop to stop and exit the function.
    auto client = ns::make_tcp_client();
    client
        ->on_connect([](auto c) {
            logi("client connected");
            ns::data x(ns::data_type::auth, 0);
            c->send(x);
        })
        ->on_read([](auto client, auto data) { logi("read data"); })
        ->on_close([](auto c) { logi("client closed"); })
        ->on_error([](auto c, const auto &e) {})
        ->run("localhost", "6666");
}
