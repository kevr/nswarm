#include "client.hpp"
#include "host/node_server.hpp"
#include "logging.hpp"
#include "memory.hpp"
#include <gtest/gtest.h>
#include <string>

class node_server_test : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        ns::set_trace_logging(true);
        trace();
        // start() bad_weak_ptr, stop() segfault
        m_server = std::make_shared<ns::host::node_server>(6666);
        m_server
            ->on_connect([](auto c) {
                logd("client connected from ", c->remote_host(), ":",
                     c->remote_port());
            })
            .use_certificate("cert.crt", "cert.key");
        m_server->start();
    }

    virtual void TearDown()
    {
        trace();
        m_server->stop();
    }

    std::shared_ptr<ns::host::node_server> m_server;
};

TEST_F(node_server_test, server_listens)
{
    trace();

    auto client = ns::make_tcp_client();
    client
        ->on_connect([](auto c) {
            logi("connected to ", c->remote_host(), ":", c->remote_port());
            c->close();
        })
        .on_close([](auto c) { logi("closed"); })
        .on_error([](auto c, const auto &e) { loge("error: ", e.message()); })
        .run("localhost", "6666");
}

TEST_F(node_server_test, server_authenticates)
{
    trace();

    auto client = ns::make_tcp_client(m_server->get_io_service());
    client
        ->on_connect([](auto c) {
            logi("connected to ", c->remote_host(), ":", c->remote_port());
            uint64_t header = ns::serialize_header(ns::data_type::auth, 0, 0);
            ns::data x(header);
            c->send(x);
        })
        .on_close([](auto c) { logi("closed"); })
        .on_error([](auto c, const auto &e) { loge("error: ", e.message()); })
        .run("localhost", "6666");

    // Wait until client is connected.
    // We need a better wait to wait until a client is added
    // on the server side.
    ns::wait_until([&] { return client->connected(); });
    EXPECT_EQ(m_server->count(), 1);
    client->close();

    ns::wait_until([&] { return !client->connected(); });
    m_server->stop();
    EXPECT_EQ(m_server->count(), 0);
}

TEST_F(node_server_test, server_denies_unauthenticated)
{
    trace();
}
