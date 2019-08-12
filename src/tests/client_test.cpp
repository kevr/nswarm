#include "server.hpp"
#include <boost/version.hpp>
#include <gtest/gtest.h>
#include <nswarm/client.hpp>
#include <nswarm/logging.hpp>

class client_test : public ::testing::Test
{
protected:
    static void SetUpTestCase()
    {
        ns::set_trace_logging(true);
    }

    virtual void SetUp()
    {
        m_server.use_certificate("cert.crt", "cert.key");
        m_server.start();
    }

    virtual void TearDown()
    {
        m_server.stop();
    }

    ns::tcp_server<ns::tcp_connection> m_server{6666};
};

TEST_F(client_test, google_search)
{
    ns::set_debug_logging(true);

    ns::io_service io;
    auto client = ns::make_tcp_client(io);

    client
        ->on_connect([](auto client) {
            logd("client connected to ", client->remote_host(), ":",
                 client->remote_port());
            client->close();
        })
        .on_read([](auto client, auto msg) {
            logd("Message received");
        })
        .on_close([](auto client) {
            logd("Client disconnected.");
        })
        .run("localhost", "6666");

    io.run();
}

TEST_F(client_test, internal_io_service)
{
    ns::set_debug_logging(true);

    auto client = ns::make_tcp_client();

    client
        ->on_connect([](auto client) {
            logd("client connected to ", client->remote_host(), ":",
                 client->remote_port());
            client->close();
        })
        .on_read([](auto client, auto msg) {
            logd("Message received");
        })
        .on_close([](auto client) {
            logd("Client disconnected.");
        })
        .on_error([](auto client, const auto &ec) {
            loge(ec.message());
        })
        .run("localhost", "6666");
}

TEST_F(client_test, two_shared_io_clients)
{
    ns::set_debug_logging(true);

    ns::io_service io;
    auto client = ns::make_tcp_client(io);

    client
        ->on_connect([](auto client) {
            logd("client connected to ", client->remote_host(), ":",
                 client->remote_port());
            client->close();
        })
        .on_read([](auto client, auto msg) {
            logd("Message received");
        })
        .on_close([](auto client) {
            logd("Client disconnected.");
        });

    auto client2 = ns::make_tcp_client(io);

    client2
        ->on_connect([](auto client) {
            logd("client2 connected to ", client->remote_host(), ":",
                 client->remote_port());
            client->close();
        })
        .on_read([](auto client, auto msg) {
            logd("Message2 received");
        })
        .on_close([](auto client) {
            logd("Client2 disconnected.");
        });

    client->run("localhost", "6666");
    client2->run("localhost", "6666");
    io.run();
}

TEST_F(client_test, invalid_host)
{
    ns::set_debug_logging(true);

    auto client = ns::make_tcp_client();

    client
        ->on_connect([](auto client) {
            logd("client connected to ", client->remote_host(), ":",
                 client->remote_port());
            client->close();
        })
        .on_read([](auto client, auto msg) {
            logd("Message received");
        })
        .on_close([](auto client) {
            logd("Client disconnected.");
        })
        .on_error([](auto client, const auto &ec) {
            EXPECT_TRUE(ec == boost::asio::error::host_not_found ||
                        ec == boost::asio::error::host_not_found_try_again);
        })
        .run("googleboogle", "443");
}

TEST_F(client_test, invalid_port)
{
    ns::set_debug_logging(true);

    auto client = ns::make_tcp_client();

    client
        ->on_connect([](auto client) {
            logd("client connected to ", client->remote_host(), ":",
                 client->remote_port());
            client->close();
        })
        .on_read([](auto client, auto msg) {
            logd("Message received");
        })
        .on_close([](auto client) {
            logd("Client disconnected.");
        })
        .on_error([](auto client, const auto &ec) {
            EXPECT_EQ(ec, boost::asio::error::service_not_found);
        })
        .run("googleboogle", "-1");
}
