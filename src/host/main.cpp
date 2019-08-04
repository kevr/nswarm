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
#include <errno.h>
#include <nswarm/logging.hpp>
#include <string>
#include <thread>

int main(int argc, const char *argv[])
{
    ns::program_options opt(argc, argv, "Daemon options");

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

    // redirect stdout to logfile if --log was provided
    if (opt.exists("log")) {
        logd("--log ", opt.get<std::string>("log"), " found");
        logd("redirecting logs to ", opt.get<std::string>("log"));
        if (!ns::cout.redirect(opt.get<std::string>("log"))) {
            loge("unable to redirect logs to ", opt.get<std::string>("log"));
            return 1;
        }
    }

    // This has to be executed after we fork
    auto setBufferMode = [&] {
        setvbuf(stdout, NULL, _IOLBF, 0);
        setvbuf(stderr, NULL, _IOLBF, 0);
    };

    if (opt.exists("daemon")) {
        // daemon(change dir to /, don't change stdout/stderr)
        if (daemon(0, 1) == -1) {
            loge("daemon failed, errno: ", errno);
            return 1;
        }
    }

    setBufferMode();

    logi(opt.name(), " started");

    // This needs to go into a run loop waiting for the server to quit
    return 0;
}
