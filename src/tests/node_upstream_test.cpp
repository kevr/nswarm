#include "host/node_server.hpp"
#include "node/upstream.hpp"
#include "task.hpp"
#include "util.hpp"
#include <gtest/gtest.h>

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
};

TEST_F(node_upstream_test, auth_works)
{
    std::chrono::high_resolution_clock hrc;
    std::chrono::time_point<std::chrono::high_resolution_clock> tp1, tp2;

    auto upstream = std::make_shared<node::upstream>(m_server.get_io_service());
    upstream->on_connect([&](auto client) {
        logi("sending auth key abcd");
        tp1 = hrc.now();
        client->auth("abcd");
    });
    upstream->run("localhost", "6666");
    ns::wait_until([&] {
        return upstream->authenticated();
    });
    tp2 = hrc.now();

    auto delta =
        std::chrono::duration_cast<std::chrono::milliseconds>(tp2 - tp1)
            .count();
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
    upstream->respond("taskUUID", ns::task_type::emit::value,
                      std::vector<std::string>());

    auto task = ns::task_type::deduce(ns::task_type::emit::value);
    ns::match(
        task,
        [](ns::task_type::call) {
            logi("call task deduced");
        },
        [](ns::task_type::emit) {
            logi("emit task deduced");
        },
        [](auto &&) {
            logi("invalid task deduced");
        });

    std::this_thread::sleep_for(std::chrono::seconds(1));
}
