#include "host/node_server.hpp"
#include "node/upstream.hpp"
#include <gtest/gtest.h>
#include <nswarm/task.hpp>
#include <nswarm/util.hpp>

using namespace ns;

class node_upstream_test : public ::testing::Test
{
protected:
    static void SetUpTestCase()
    {
        ns::set_trace_logging(true);
    }

    virtual void SetUp()
    {
        trace();
        m_server.use_certificate("cert.crt", "cert.key");
        m_server.set_auth_key("abcd");
        m_server.start();
    }

    virtual void TearDown()
    {
        trace();
        m_server.stop();
    }

    host::node_server m_server{6666};
    util::benchmark bench;
};

TEST_F(node_upstream_test, auth_works)
{
    double delta; // benchmark value

    auto upstream = std::make_shared<node::upstream>(m_server.get_io_service());
    upstream
        ->on_connect([&](auto client) {
            logi("sending auth key abcd");
            bench.start();
            client->auth("abcd");
        })
        .on_auth([&](auto client, auto message) {
            delta = bench.stop();
        });

    upstream->run("localhost", "6666");
    ns::wait_until([&] {
        return upstream->authenticated();
    });

    logi("authentication took: ", delta, "ms");
}

TEST_F(node_upstream_test, implements)
{
    double delta; // benchmark value

    using ns::net::action;

    // Here we just send a response as if it was successful
    m_server.set_auth_key("AuthKey");
    m_server.on_implement([&](auto node, auto impl) {
        impl.update_action(action::type::response);
        node->send(std::move(impl));
    });

    // A thread-safe functor. Any data operated on within
    // this functor will be accessed acrossed mutex locks.
    auto guarded = ns::util::guard();
    net::implementation impl;

    auto upstream = std::make_shared<node::upstream>(m_server.get_io_service());
    upstream
        ->on_connect([&](auto client) {
            client->auth("AuthKey");
        })
        .on_auth([&](auto client, auto message) {
            // Send implements request
            bench.start();
            logi("successfully authenticated with key: ", message.key());
            client->implement("test");
        })
        .on_implement([&](auto client, auto message) {
            delta = bench.stop();
            EXPECT_FALSE(message.has_error());
            EXPECT_EQ(message.method(), "test");
            guarded([&] {
                impl = message;
            });
        });

    upstream->run("localhost", "6666");
    ns::wait_until([&] {
        return guarded([&] {
            return impl.get_action() == action::type::response;
        });
    });

    logi("implementation took: ", delta, "ms");
}

TEST_F(node_upstream_test, subscribes)
{
}

