#include "client.hpp"
#include "logging.hpp"
#include <gtest/gtest.h>

TEST(client_test, google_search)
{
    ns::cout.set_debug(true);
    logi("Starting client_test...");

    ns::io_service io;
    ns::tcp_client client(io);

    client
        .on_connect([](auto &client) {
            logd("Client connected!");
            client.close();
        })
        .on_read([](auto &client, auto msg) { logd("Message received"); })
        .on_close([](auto &client) { logd("Client disconnected."); });

    client.run("www.google.com", "443");
    io.run();
}

TEST(client_test, internal_io_service)
{
    ns::cout.set_debug(true);
    logi("Starting client_test...");

    ns::tcp_client client;

    client
        .on_connect([](auto &client) {
            logd("Client connected!");
            client.close();
        })
        .on_read([](auto &client, auto msg) { logd("Message received"); })
        .on_close([](auto &client) { logd("Client disconnected."); })
        .on_error([](auto &client, const auto &ec) { loge(ec.message()); });

    client.run("www.google.com", "443");
}

TEST(client_test, two_shared_io_clients)
{
    ns::cout.set_debug(true);
    logi("Starting client_test...");

    ns::io_service io;
    ns::tcp_client client(io);

    client
        .on_connect([](auto &client) {
            logd("Client connected!");
            client.close();
        })
        .on_read([](auto &client, auto msg) { logd("Message received"); })
        .on_close([](auto &client) { logd("Client disconnected."); });

    ns::tcp_client client2(io);

    client2
        .on_connect([](auto &client) {
            logd("Client2 connected!");
            client.close();
        })
        .on_read([](auto &client, auto msg) { logd("Message2 received"); })
        .on_close([](auto &client) { logd("Client2 disconnected."); });

    client.run("www.google.com", "443");
    client2.run("www.google.com", "443");
    io.run();
}

TEST(client_test, unable_to_connect)
{
    ns::cout.set_debug(true);
    logi("Starting client_test...");

    ns::tcp_client client;

    client
        .on_connect([](auto &client) {
            logd("Client connected!");
            client.close();
        })
        .on_read([](auto &client, auto msg) { logd("Message received"); })
        .on_close([](auto &client) { logd("Client disconnected."); })
        .on_error([](auto &client, const auto &ec) { loge(ec.message()); });

    client.run("googleboogle", "443");
}

TEST(client_test, invalid_port)
{
    ns::cout.set_debug(true);
    logi("Starting client_test...");

    ns::tcp_client client;

    client
        .on_connect([](auto &client) {
            logd("Client connected!");
            client.close();
        })
        .on_read([](auto &client, auto msg) { logd("Message received"); })
        .on_close([](auto &client) { logd("Client disconnected."); })
        .on_error([](auto &client, const auto &ec) { loge(ec.message()); });

    client.run("googleboogle", "-1");
}
