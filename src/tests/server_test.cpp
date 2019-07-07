#include "client.hpp"
#include "server.hpp"
#include <gtest/gtest.h>

class server_test : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        trace();

        ns::set_debug_logging(true);

        m_server = ns::make_tcp_server(6666);

        // Setup connection bindings
        m_server->use_certificate("cert.crt", "cert.key")
            .on_accept([](auto client) {
                logi("connection accepted");
                client->run();
            })
            .on_server_error([](auto server, const auto &ec) {
                loge("server error: ", ec.message());
            })
            .on_connect([](auto client) {
                logi("client connected from ", client->remote_host(), ":",
                     client->remote_port());
            })
            .on_read([](auto client, auto data) {
                EXPECT_EQ(data.type(), ns::data_type::auth);
                EXPECT_EQ(data.size(), data.get_string().size());
                logi("read data type: ", data.type());
                client->close();
            })
            .on_close([](auto client) { logi("connection closed"); })
            .on_error([](auto client, const auto &ec) {
                loge("error: ", ec.message());
            })
            .start();
    }

    virtual void TearDown()
    {
        trace();
        m_server->stop();
    }

    std::shared_ptr<ns::tcp_server<ns::tcp_connection>> m_server;
};

TEST_F(server_test, server_accepts_client)
{
    trace();

    EXPECT_EQ(m_server->count(), 0);

    // Connect to our m_server, and close the connection when we connect.
    // This will cause the run loop to stop and exit the function.
    auto client = ns::make_tcp_client();
    client
        ->on_connect([&](auto c) {
            logi("client connected to ", c->remote_host(), ":",
                 c->remote_port());
            EXPECT_EQ(m_server->count(), 1);
            c->close();
        })
        .on_read([](auto client, auto data) { logi("read data"); })
        .on_close([](auto c) { logi("client closed"); })
        .on_error([](auto c, const auto &e) {})
        .run("localhost", "6666");

    EXPECT_EQ(m_server->count(), 0);
}

TEST_F(server_test, server_serializes_properly)
{
    trace();

    // Connect to our m_server, and close the connection when we connect.
    // This will cause the run loop to stop and exit the function.
    auto client = ns::make_tcp_client();
    client
        ->on_connect([](auto c) {
            logi("client connected to ", c->remote_host(), ":",
                 c->remote_port());
            ns::data x(ns::data_type::auth, 0);
            c->send(x);
        })
        .on_read([](auto client, auto data) { logi("read data"); })
        .on_close([](auto c) { logi("client closed"); })
        .on_error([](auto c, const auto &e) {})
        .run("localhost", "6666");
}
