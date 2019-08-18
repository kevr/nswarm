/**
 * Project: nswarm
 * File: host/main.cpp
 *
 * Main entry point for the nswarm host daemon.
 *
 * Copyright (c) 2019 Kevin Morris
 * All Rights Reserved
 **/
#include "config.hpp"
#include "host/daemon.hpp"

#include <errno.h>
#include <nswarm/logging.hpp>
#include <string>
#include <thread>

int main(int argc, const char *argv[])
{
    ns::program_options opt("Daemon options");

    // Add nswarm-host specific options
    opt.add_required_option<std::string>("api-cert",
                                         "SSL certificate for api server");
    opt.add_required_option<std::string>("api-cert-key",
                                         "SSL certificate key for api server");
    opt.add_required_option<std::string>(
        "api-auth-key", "Authentication key required by incoming API users");

    opt.add_required_option<std::string>("node-cert",
                                         "SSL certificate for node server");
    opt.add_required_option<std::string>("node-cert-key",
                                         "SSL certificate key for node server");
    opt.add_required_option<std::string>(
        "node-auth-key", "Authentication key required by incoming nodes");

    std::string home_config(getenv("HOME"));
    home_config.append("/.nswarm-host.conf");
    parse_configs(opt, home_config, "/etc/nswarm-host.conf");

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

    ns::host::daemon daemon;

    const auto &node_cert = opt.get<std::string>("node-cert");
    const auto &node_cert_key = opt.get<std::string>("node-cert-key");
    const auto &node_auth_key = opt.get<std::string>("node-auth-key");

    // Configure node server certificates
    daemon.set_node_certificate(node_cert, node_cert_key);

    // Configure node server authentication key
    daemon.set_node_auth_key(node_auth_key);

    // Configure api server certificates
    // Configure api server authentication key
    return daemon.run();
}
