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

    if (opt.exists("help")) {
        std::cout << opt.usage() << std::endl << std::endl << opt;
        return 1;
    }

    // Enable debug if -d provided
    if (opt.exists("debug")) {
        ns::cout.set_debug(true);
    }

    return 0;
}
