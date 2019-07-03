/**
 * \project nswarm
 * \file tests/logging_test.cpp
 *
 * \description Test cases for logging tools.
 *
 * Copyright (c) 2019 Kevin Morris
 * All Rights Reserved.
 **/
#include "logging.hpp"
#include <gtest/gtest.h>
#include <thread>

TEST(logging_test, logstream)
{
    ns::cout.set_debug(true);

    std::vector<std::shared_ptr<std::thread>> jobs = {
        std::make_shared<std::thread>(
            [&] { logi("Test1", " Test2", " Test3", " Test4"); }),
        std::make_shared<std::thread>(
            [&] { logd("Test2", " Test2", " Test3", " Test4"); }),
        std::make_shared<std::thread>(
            [&] { loge("Test3", " Test2", " Test3", " Test4"); })};

    for (auto &job : jobs)
        job->join();
}
