/**
 * \project nswarm
 * \file tests/sensor_test.cpp
 *
 * \description Test cases for sensor objects.
 *
 * Copyright (c) 2019 Kevin Morris
 * All Rights Reserved.
 **/
#include "logging.hpp"
#include "memory.hpp"
#include <gtest/gtest.h>
#include <thread>

TEST(sensor_test, memory_sensor)
{
    ns::set_trace_logging(true);

    ns::memory_sensor mem_sensor(1000);
    logd("Memory sensor value: ", mem_sensor.get_value());

    mem_sensor.start();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    logd("Memory sensor value: ", mem_sensor.get_value());

    mem_sensor.stop();
}
