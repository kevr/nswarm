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

#include "logging.hpp"
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

}; // namespace ns

#endif
