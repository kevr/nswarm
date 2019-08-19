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

void add_options(ns::program_options &opt);

int main(int argc, const char *argv[])
{
    std::string home_config(getenv("HOME"));
    home_config.append("/.nswarm-host.conf");

    ns::program_options opt("Daemon options");
    add_options(opt);
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

    const int node_port =
        opt.exists("node-server-port")
            ? std::stoi(opt.get<std::string>("node-server-port"))
            : 6666;
    const int api_port =
        opt.exists("api-server-port")
            ? std::stoi(opt.get<std::string>("api-server-port"))
            : 6667;

    const auto node_cert = opt.get<std::string>("node-cert");
    const auto node_cert_key = opt.get<std::string>("node-cert-key");
    const auto node_auth_key = opt.get<std::string>("node-auth-key");

    ns::host::daemon daemon(api_port, node_port);
    daemon.set_node_certificate(node_cert, node_cert_key);
    daemon.set_node_auth_key(node_auth_key);
    return daemon.run();
}

// Specific nswarm-host options, as well as some optionals
// that are also specific to nswarm-host
void add_options(ns::program_options &opt)
{
    opt.add_required_option<std::string>("api-cert",
                                         "SSL certificate for api server");
    opt.add_required_option<std::string>("api-cert-key",
                                         "SSL certificate key for api server");
    opt.add_required_option<std::string>(
        "api-auth-key", "Authentication key required by incoming API users");
    opt.add_option<std::string>("api-server-host",
                                "Bind host for the tcp api server");
    opt.add_option<std::string>("api-server-port",
                                "Listening port for the tcp api server");

    opt.add_required_option<std::string>("node-cert",
                                         "SSL certificate for node server");
    opt.add_required_option<std::string>("node-cert-key",
                                         "SSL certificate key for node server");
    opt.add_required_option<std::string>(
        "node-auth-key", "Authentication key required by incoming nodes");
    opt.add_option<std::string>("node-server-host",
                                "Bind host for the tcp node server");
    opt.add_option<std::string>("node-server-port",
                                "Listening port for the tcp node server");
}
