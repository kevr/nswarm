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
