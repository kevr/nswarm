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
#include "logging.hpp"
#include <string>

int main(int argc, const char *argv[])
{
    ns::program_options opt(argc, argv, "Daemon options");

    // If unable to parse or -h was provided, print help output
    if (!opt || opt.exists("help")) {
        return opt.help();
    }

    // initialize debug logging if -d was provided
    if (opt.exists("debug")) {
        ns::cout.set_debug(true);
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

    logi("logging started");
    return 0;
}
