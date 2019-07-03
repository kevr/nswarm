#include "config.hpp"
#include <gtest/gtest.h>

TEST(program_options, executable_name)
{
    int argc = 1;
    const char *argv[1] = {
        "./program_options_test",
    };
    ns::program_options options(argc, argv);

    EXPECT_EQ(options.name(), "program_options_test");
}

TEST(program_options, abs_executable_name)
{
    int argc = 1;
    const char *argv[1] = {
        "/usr/bin/program_options_test",
    };
    ns::program_options options(argc, argv);

    EXPECT_EQ(options.name(), "program_options_test");
}
