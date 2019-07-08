#include "host/node_server.hpp"
#include "node/upstream.hpp"
#include <gtest/gtest.h>

using namespace ns;

class node_upstream_test : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        set_trace_logging(true);
        trace();
        m_server.use_certificate("cert.crt", "cert.key").start();
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
    std::this_thread::sleep_for(std::chrono::seconds(10));
}
