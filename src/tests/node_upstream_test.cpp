#include "host/node_server.hpp"
#include "node/upstream.hpp"
#include "task.hpp"
#include "util.hpp"
#include <gtest/gtest.h>

using namespace ns;

class node_upstream_test : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        set_trace_logging(true);
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
};

TEST_F(node_upstream_test, auth_works)
{
    auto upstream = std::make_shared<node::upstream>(m_server.get_io_service());
    upstream->on_connect([this](auto client) {
        logi("sending auth key abcd");
        client->auth("abcd");
    });
    upstream->run("localhost", "6666");
    ns::wait_until([&] { return upstream->authenticated(); });
}

TEST_F(node_upstream_test, task_works)
{
    // First, authenticate on a client. Then send a task.
    using namespace ns::tasks;

    ns::data task_msg;
    {
        auto task = tasks::task<tasks::emit>("taskUUID");
        auto task_str = task.serialize(json::parse("{\"event\":\"test\"}"));
        task_msg = ns::data(
            serialize_header(ns::data_type::task, task.type(), task_str.size()),
            task_str);
    }

    auto upstream = std::make_shared<node::upstream>(m_server.get_io_service());
    upstream->on_connect([this](auto client) {
        logi("sending auth key abcd");
        client->auth("abcd");
    });
    upstream->run("localhost", "6666");
    ns::wait_until([&] { return upstream->authenticated(); });
    upstream->send(task_msg);
    std::this_thread::sleep_for(std::chrono::seconds(1));
}
