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
    auto upstream = std::make_shared<node::upstream>(m_server.get_io_service());
    upstream->on_connect([&](auto client) {
        logi("sending auth key abcd");
        bench.start();
        client->auth("abcd");
    });

    upstream->run("localhost", "6666");
    ns::wait_until([&] {
        return upstream->authenticated();
    });

    auto delta = bench.stop();
    logi("Authentication took: ", delta, "ms");
}

TEST_F(node_upstream_test, task_response)
{
    // First, authenticate on a client. Then send a task.

    auto upstream = std::make_shared<node::upstream>(m_server.get_io_service());
    upstream->on_connect([this](auto client) {
        logi("sending auth key abcd");
        client->auth("abcd");
    });
    upstream->run("localhost", "6666");
    // authenticated() is a higher level, thread safe function
    ns::wait_until([&] {
        return upstream->authenticated();
    });
    // upstream->respond("taskUUID", net::task::event,
    // std::vector<std::string>());

    auto task = net::task::deduce(net::task::event);
    ns::match(
        task,
        [](net::task::tag::call) {
            logi("call task deduced");
        },
        [](net::task::tag::event) {
            logi("emit task deduced");
        },
        [](auto &&) {
            logi("invalid task deduced");
        });

    std::this_thread::sleep_for(std::chrono::seconds(1));
}

TEST_F(node_upstream_test, implements)
{
    // First, authenticate on a client. Then send a task.

    auto upstream = std::make_shared<node::upstream>(m_server.get_io_service());
    upstream->on_connect([this](auto client) {
        logi("sending auth key abcd");
        client->auth("abcd");
    });
    upstream->run("localhost", "6666");
    // authenticated() is a higher level, thread safe function
    ns::wait_until([&] {
        return upstream->authenticated();
    });

    logi("implementing method1");
    upstream->implement("method1");

    std::this_thread::sleep_for(std::chrono::seconds(1));
}
