#include "host/node_server.hpp"
#include "logging.hpp"
#include <gtest/gtest.h>

class node_server_test : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        ns::set_debug_logging(true);
        // start() bad_weak_ptr, stop() segfault
        m_server = std::make_shared<ns::host::node_server>(m_io, 6666);
        m_server->start();
    }

    virtual void TearDown()
    {
        m_server->stop();
    }

    ns::io_service m_io;
    std::shared_ptr<ns::host::node_server> m_server;
};

TEST_F(node_server_test, server_listens)
{
}

TEST_F(node_server_test, server_authenticates)
{
}

TEST_F(node_server_test, server_denies_unauthenticated)
{
}
