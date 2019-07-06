/**
 * \project nswarm
 * \file tests/program_options_test.cpp
 * \description program_options class tests for parsing user arguments
 *
 * Copyright (c) 2019 Kevin Morris
 * All Rights Reserved.
 **/
#include "config.hpp"
#include "logging.hpp"
#include <gtest/gtest.h>

TEST(program_options, executable_name)
{
    ns::cout.set_debug(true);
    int argc = 1;
    const char *argv[1] = {
        "./program_options_test",
    };
    ns::program_options options(argc, argv, "program_options_test");

    EXPECT_EQ(options.name(), "program_options_test");
    logd("Found executable name: ", options.name());
}

TEST(program_options, abs_executable_name)
{
    ns::cout.set_debug(true);
    int argc = 1;
    const char *argv[1] = {
        "/usr/bin/program_options_test",
    };
    ns::program_options options(argc, argv, "program_options_test");

    EXPECT_EQ(options.name(), "program_options_test");
    logd("Found executable name: ", options.name());
}
