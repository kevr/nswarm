#include "client.hpp"
#include "logging.hpp"
#include <gtest/gtest.h>

TEST(client_test, google_search)
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
        .on_read([](auto client, auto msg) { logd("Message received"); })
        .on_close([](auto client) { logd("Client disconnected."); })
        .run("www.google.com", "443");

    io.run();
}

TEST(client_test, internal_io_service)
{
    ns::set_debug_logging(true);

    auto client = ns::make_tcp_client();

    client
        ->on_connect([](auto client) {
            logd("client connected to ", client->remote_host(), ":",
                 client->remote_port());
            client->close();
        })
        .on_read([](auto client, auto msg) { logd("Message received"); })
        .on_close([](auto client) { logd("Client disconnected."); })
        .on_error([](auto client, const auto &ec) { loge(ec.message()); })
        .run("www.google.com", "443");
}

TEST(client_test, two_shared_io_clients)
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
        .on_read([](auto client, auto msg) { logd("Message received"); })
        .on_close([](auto client) { logd("Client disconnected."); });

    auto client2 = ns::make_tcp_client(io);

    client2
        ->on_connect([](auto client) {
            logd("client2 connected to ", client->remote_host(), ":",
                 client->remote_port());
            client->close();
        })
        .on_read([](auto client, auto msg) { logd("Message2 received"); })
        .on_close([](auto client) { logd("Client2 disconnected."); });

    client->run("www.google.com", "443");
    client2->run("www.google.com", "443");
    io.run();
}

TEST(client_test, unable_to_connect)
{
    ns::set_debug_logging(true);

    auto client = ns::make_tcp_client();

    client
        ->on_connect([](auto client) {
            logd("client connected to ", client->remote_host(), ":",
                 client->remote_port());
            client->close();
        })
        .on_read([](auto client, auto msg) { logd("Message received"); })
        .on_close([](auto client) { logd("Client disconnected."); })
        .on_error([](auto client, const auto &ec) {
            EXPECT_EQ(ec, boost::asio::error::host_not_found);
        })
        .run("googleboogle", "443");
}

TEST(client_test, invalid_port)
{
    ns::set_debug_logging(true);

    auto client = ns::make_tcp_client();

    client
        ->on_connect([](auto client) {
            logd("client connected to ", client->remote_host(), ":",
                 client->remote_port());
            client->close();
        })
        .on_read([](auto client, auto msg) { logd("Message received"); })
        .on_close([](auto client) { logd("Client disconnected."); })
        .on_error([](auto client, const auto &ec) {
            EXPECT_EQ(ec, boost::asio::error::service_not_found);
        })
        .run("googleboogle", "-1");
}
