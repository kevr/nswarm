/**
 * Project: nswarm
 * File: main.cpp
 *
 * Main entry point for the nswarm cluster daemon.
 *
 * Copyright (c) 2019 Kevin Morris
 * All Rights Reserved
**/
#include "config.hpp"
#include "logging.hpp"
#include <string>

int main(int argc, const char *argv[])
{
    ns::program_options opt(argc, argv);
    ns::detail::logstream log_;
    log_.out("Test ", "me");
    return 0;
}

