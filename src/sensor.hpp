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
#include "util.hpp"
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
    sensor(int64_t interval_in_milliseconds)
        : m_interval(interval_in_milliseconds)
    {
        logd("sensor initialized with interval = ", m_interval, "ms");
    }

    virtual ~sensor() = default;

    void start()
    {
        // m_running = true;
        m_running.exchange(true);
        m_thread = std::thread([this]() { this->work_loop(); });
    }

    void stop()
    {
        // m_running = false;
        m_running.exchange(false);
        // use an interrupt
        m_thread.join();
    }

    T get_value()
    {
        logd("acquiring get_value mutex");
        ns::lock_guard<std::mutex> g(m_mutex);
        logd("mutex acquired, continuing");
        return get();
    }

protected:
    virtual void work() = 0;
    virtual void locked() = 0;
    virtual T get() = 0;

private:
    void work_loop()
    {
        trace();

        while (true) {
            {
                logd("reached work interval, calling pre-work");
                work();

                logd("work done, acquiring mutex");
                ns::lock_guard<std::mutex> g(m_mutex);
                logd("mutex acquired, dispersing data");
                locked();
                logd("dispersed locked data");
            }

            int64_t r = m_interval; // interval total
            while (r >= 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                r -= 10; // take away 10ms
                if (!m_running.load()) {
                    return;
                }
            }
        }
    }

private:
    std::mutex m_mutex;
    std::thread m_thread;

    std::atomic<bool> m_running;

    int64_t m_interval;

    set_log_address;
};

}; // namespace ns

#endif
