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

    // Search through all common config file paths, and load them
    // in priority /etc, then /home
    auto configs = ns::util::any_file(home_config, "/etc/nswarm-host.conf");
    for (auto &config : configs) {
        opt.parse_config(config);
        logi("loaded configuration file: ", config);
    }

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
            loge("daemon failed, errno: ", errno);
            return 1;
        }
    }

    // Set line buffering in the current process
    ns::set_buffer_mode(ns::line_buffering);

    // Begin real program logic
    logi(opt.name(), " started");

    ns::host::daemon daemon;

    // Configure daemon
    daemon.use_node_certificate(opt.get<std::string>("node-cert"),
                                opt.get<std::string>("node-cert-key"));
    daemon.use_node_auth_key(opt.get<std::string>("node-auth-key"));

    return daemon.run();
}
