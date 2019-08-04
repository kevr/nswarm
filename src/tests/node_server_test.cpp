#include "host/node_server.hpp"
#include "memory.hpp"
#include <gtest/gtest.h>
#include <nswarm/client.hpp>
#include <nswarm/logging.hpp>
#include <string>

using namespace ns;

class node_server_test : public ::testing::Test
{
protected:
    static void SetUpTestCase()
    {
        ns::set_trace_logging(true);
    }

    virtual void SetUp()
    {
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
        .on_close([](auto c) {
            logi("closed");
        })
        .on_error([](auto c, const auto &e) {
            loge("error: ", e.message());
        })
        .run("localhost", "6666");
}

TEST_F(node_server_test, server_denies_auth)
{
    trace();

    auto client = ns::make_tcp_client();
    client
        ->on_connect([this](auto c) {
            EXPECT_EQ(m_server->count(), 1);
            logi("connected to ", c->remote_host(), ":", c->remote_port());
            c->send(net::make_auth_request(ns::json{{"key", "abcd"}}));
        })
        .on_close([this](auto c) {
            logi("closed");
        })
        .on_error([this](auto c, const auto &e) {
            loge("error: ", e.message());
        })
        .run("localhost", "6666");

    // Wait until client is connected.
    // We need a better wait to wait until a client is added
    // on the server side.
    ns::wait_until([&] {
        return m_server->count() == 0;
    });
}

TEST_F(node_server_test, server_approves_auth)
{
    trace();
}
