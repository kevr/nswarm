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
    double delta; // benchmark value

    using ns::net::action;

    // Here we just send a response as if it was successful
    m_server.set_auth_key("AuthKey");
    m_server.on_subscribe([&](auto node, auto sub) {
        sub.update_action(action::type::response);
        node->send(std::move(sub));
    });

    // A thread-safe functor. Any data operated on within
    // this functor will be accessed acrossed mutex locks.
    auto guarded = ns::util::guard();
    net::subscription sub;

    auto upstream = std::make_shared<node::upstream>(m_server.get_io_service());
    upstream
        ->on_connect([&](auto client) {
            client->auth("AuthKey");
        })
        .on_auth([&](auto client, auto message) {
            // Send implements request
            bench.start();
            logi("successfully authenticated with key: ", message.key());
            client->subscribe("test");
        })
        .on_subscribe([&](auto client, auto message) {
            delta = bench.stop();
            EXPECT_FALSE(message.has_error());
            EXPECT_EQ(message.event(), "test");
            guarded([&] {
                sub = message;
            });
        });

    upstream->run("localhost", "6666");
    ns::wait_until([&] {
        return guarded([&] {
            return sub.get_action() == action::type::response;
        });
    });

    logi("subscription took: ", delta, "ms");
}

TEST_F(node_upstream_test, handles_task)
{
    double delta; // benchmark value

    using ns::net::action;

    net::task t;
    auto guarded = ns::util::guard();

    auto upstream = std::make_shared<node::upstream>(m_server.get_io_service());

    // Here we just send a response as if it was successful
    m_server.set_auth_key("AuthKey");
    m_server.on_auth([&](auto node, auto message) {
        logi("node authentication state: ", node->authenticated());
        auto task = ns::net::make_task_request("taskUUID");
        task.set_method("test");
        bench.start();
        node->send(task); // This never gets called on upstream's side
        // Why? We need to investigate the bug here.
        // It seems to never even read the async_on_read_header
        // portion at any point. Perhaps there's a race
        // with signaling here?...
    });
    m_server.on_task([&](auto node, auto task) {
        delta = bench.stop();
        EXPECT_EQ(task.get_action(), action::type::response);
        EXPECT_EQ(task.head().args(), ns::net::task::type::call);
        EXPECT_EQ(task.get_method(), "test");
        guarded([&] {
            t = task;
        });
    });

    upstream
        ->on_connect([&](auto client) {
            client->auth("AuthKey");
        })
        .on_auth([&](auto client, auto message) {
            logi("successfully authenticated with key: ", message.key());
        })
        .on_task([](auto client, auto task) {
            task.update_action(action::type::response);

            // An empty list
            auto json = task.get_json();
            json["data"] = std::vector<std::string>();
            task.update(json);

            client->send(task);
        });

    upstream->run("localhost", "6666");
    ns::wait_until([&] {
        return guarded([&] {
            return t.get_action() == action::type::response;
        });
    });

    logi("task took: ", delta, "ms");
}
