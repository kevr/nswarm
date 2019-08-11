/**
 * Project: nswarm
 * File: node/main.cpp
 *
 * Main entry point for the node service daemon. The node
 * daemon hosts a local service server for apps to implement
 * methods or subscribe to events. The node daemon acts as
 * a management layer between services and the host.
 *
 * Copyright (c) 2019 Kevin Morris
 * All Rights Reserved
 **/
#include "config.hpp"
#include "node/upstream.hpp"

#include <errno.h>
#include <nswarm/logging.hpp>
#include <string>
#include <thread>

int main(int argc, const char *argv[])
{
    ns::program_options opt("General options");

    opt.add_required_option<std::string>("upstream-host", "Upstream hostname");
    opt.add_option<std::string>("upstream-port",
                                "Upstream port (default: 6666)");
    opt.add_required_option<std::string>(
        "upstream-auth-key", "Key used to authenticate with upstream");

    opt.add_required_option<std::string>(
        "service-cert", "SSL certificate used with service server");
    opt.add_required_option<std::string>(
        "service-cert-key", "SSL certificate key used with service server");
    opt.add_required_option<std::string>(
        "service-auth-key", "Authentication key used for node services");

    std::string home_config(getenv("HOME"));
    home_config.append("/.nswarm-node.conf");
    parse_configs(opt, home_config, "/etc/nswarm-node.conf");

    opt.parse(argc, argv);

    // If unable to parse or -h was provided, print help output
    if (!opt || opt.exists("help")) {
        return opt.help();
    }

    if (opt.exists("trace")) {
        // initialize trace (+debug) logging if --trace was provided
        ns::set_trace_logging(true);
    } else if (opt.exists("debug")) {
        // initialize debug logging if -d was provided
        ns::set_debug_logging(true);
    }

    // Make sure we have right parameter combinations
    if (opt.exists("daemon")) {
        if (!opt.exists("log")) {
            loge("--log required when daemonizing");
            return opt.help();
        }
    }

    // We should redirect to the provided log before we daemonize
    if (opt.exists("log")) {
        logd("redirecting logs to ", opt.get<std::string>("log"));
        if (!ns::cout.redirect(opt.get<std::string>("log"))) {
            loge("unable to redirect logs to ", opt.get<std::string>("log"));
            return 1;
        }
    }

    if (opt.exists("daemon")) {
        // daemon(change dir to /, don't change stdout/stderr)
        if (daemon(0, 1) == -1) {
            loge("daemon(0, 1) failed, errno: ", errno);
            return 1;
        }
    }

    // Set line buffering in the current process
    ns::set_buffer_mode(ns::line_buffering);

    // Begin real program logic
    logi(opt.name(), " started");

    ns::io_service io;
    auto upstream = std::make_shared<ns::node::upstream>(io);
    upstream->on_connect(
        [key = opt.get<std::string>("upstream-auth-key")](auto c) {
            logi("Upstream connected; sending authentication key");
            c->auth(key);
        });
    upstream->run(opt.get<std::string>("upstream-host"),
                  opt.get<std::string>("upstream-port"));
    io.run();
    return 0;
}
