#include "client.hpp"
#include "host/node_server.hpp"
#include "logging.hpp"
#include <gtest/gtest.h>
#include <string>

class node_server_test : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        ns::set_debug_logging(true);
        // start() bad_weak_ptr, stop() segfault
        m_server = std::make_shared<ns::host::node_server>(6666);
        m_server->use_certificate("cert.crt", "cert.key");
        m_server->start();
    }

    virtual void TearDown()
    {
        m_server->stop();
    }

    std::shared_ptr<ns::host::node_server> m_server;
};

TEST_F(node_server_test, server_listens)
{
    auto client = ns::make_tcp_client();
    client
        ->on_connect([](auto c) {
            logi("connected");
            uint64_t packet = ns::serialize_packet(ns::data_type::auth, 0, 0);
            ns::data x(packet);
            c->send(x);
        })
        .on_close([](auto c) { logi("closed"); })
        .on_error([](auto c, const auto &e) { loge("error: ", e.message()); })
        .run("localhost", "6666");
}

TEST_F(node_server_test, server_authenticates)
{
    auto client = ns::make_tcp_client();
    client
        ->on_connect([](auto c) {
            logi("connected");
            uint64_t packet = ns::serialize_packet(ns::data_type::task, 0, 0);
            ns::data x(packet);
            c->send(x);
        })
        .on_close([](auto c) { logi("closed"); })
        .on_error([](auto c, const auto &e) { loge("error: ", e.message()); })
        .run("localhost", "6666");
}

TEST_F(node_server_test, server_denies_unauthenticated)
{
}
