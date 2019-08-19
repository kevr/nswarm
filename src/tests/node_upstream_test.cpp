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

TEST_F(node_upstream_test, task_response)
{
}

TEST_F(node_upstream_test, implements)
{
}
