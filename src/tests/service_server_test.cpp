#include "node/service.hpp"
#include <gtest/gtest.h>

class service_server_test : public ::testing::Test
{
protected:
    static void SetUpTestCase()
    {
        ns::set_trace_logging(true);
    }

    virtual void SetUp()
    {
        m_server.use_certificate("cert.crt", "cert.key").start();
    }

    virtual void TearDown()
    {
        m_server.stop();
    }

    ns::node::service_server m_server{6666};
};

TEST_F(service_server_test, service_server_listens)
{
}

