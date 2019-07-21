/**
 * \prject nswarm
 * \file util.hpp
 * \brief Convenient utility functions and structures.
 *
 * Copyright (c) 2019 Kevin Morris
 * All Rights Reserved.
 **/

#ifndef NS_UTIL_HPP
#define NS_UTIL_HPP

#include <nswarm/logging.hpp>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace ns
{
//
// A wrapped std::lock_guard, providing debug logging
// so we can follow the guards.
//
template <typename T>
class lock_guard
{
public:
    lock_guard(T &arg)
        : m_guard(arg)
    {
        logd("acquired");
    }

    ~lock_guard()
    {
        logd("released");
    }

private:
    std::lock_guard<T> m_guard;
};

// Synchronization tools
template <typename Predicate>
void wait_until(Predicate p, int64_t timeout = 60)
{
    logd("waiting ", timeout, " seconds until predicate is true");
    int64_t timeout_ms = 60 * 1000;
    while (timeout_ms > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (p()) {
            logd("predicate matched");
            return;
        }
        timeout_ms -= 1;
    }
    throw std::out_of_range(
        "wait_until timeout reached: " + std::to_string(timeout) + " seconds");
}

namespace util
{
// Benchmark tools
class benchmark
{
    using hrc = std::chrono::high_resolution_clock;
    hrc m_hrc;
    hrc::time_point m_begin;
    hrc::time_point m_end;

public:
    void start()
    {
        m_begin = m_hrc.now();
    }

    double stop()
    {
        m_end = m_hrc.now();
        return std::chrono::duration_cast<std::chrono::microseconds>(m_end -
                                                                     m_begin)
                   .count() /
               1000.0;
    }
};
}; // namespace util

}; // namespace ns

#endif
