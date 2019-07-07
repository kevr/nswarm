/**
 * \prject nswarm
 * \file sensor.hpp
 * \brief Sensory data about the process environment.
 *
 * Copyright (c) 2019 Kevin Morris
 * All Rights Reserved.
 **/
#ifndef NS_SENSOR_HPP
#define NS_SENSOR_HPP

#include "logging.hpp"
#include <atomic>
#include <mutex>
#include <string>
#include <thread>

namespace ns
{

template <typename T>
class sensor
{
public:
    sensor(uint64_t interval_in_milliseconds)
        : m_interval(interval_in_milliseconds)
    {
        logd("sensor initialized with interval = ", m_interval, "ms");
    }

    virtual ~sensor() = default;

    void start()
    {
        m_running = true;
        m_thread = std::thread([this]() { this->work_loop(); });
    }

    void stop()
    {
        m_running = false;
        // use an interrupt
        m_thread.join();
    }

    T get_value()
    {
        std::lock_guard<std::mutex> g(m_mutex);
        return get();
    }

protected:
    virtual void work() = 0;

    virtual T get() = 0;

private:
    void work_loop()
    {
        trace();

        bool keep_running = true;
        while (keep_running) {
            // Once every 10 seconds.
            int r = m_interval;
            while (r >= 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                r -= 10;
                if (!m_running) {
                    keep_running = false;
                    // Check our atomic flag for interrupt.
                    break;
                }
            }
            if (!keep_running)
                break;
            logd("reached work interval, acquiring mutex");
            std::lock_guard<std::mutex> g(m_mutex);
            logd("mutex acquired, working");
            work();
            logd("work done");
        }
    }

private:
    std::mutex m_mutex;
    std::thread m_thread;

    std::atomic<bool> m_running;

    uint64_t m_interval;

    set_log_address;
};

}; // namespace ns

#endif
