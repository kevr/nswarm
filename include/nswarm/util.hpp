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

#include <mutex>
#include <stdexcept>
#include <sys/stat.h>
#include <thread>
#include <vector>

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
    }

    ~lock_guard()
    {
    }

private:
    std::lock_guard<T> m_guard;
};

// Synchronization tools
template <typename Predicate>
void wait_until(Predicate p, int64_t timeout = 60)
{
    int64_t timeout_ms = 60 * 1000;
    while (timeout_ms > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (p()) {
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

inline bool file_exists(const std::string &path)
{
    struct stat buf;
    return stat(path.c_str(), &buf) == 0;
}

// These three functions will construct a vector of paths that
// we found to exist on the filesystem.
//
// For example: any_file("a", "b") -> std::vector{"a", "b"} if a and b
// are real files in the filesystem. any_file("a", "b") -> std::vector{"a"} if
// a exists but b does not.
//
template <typename... Args>
std::vector<std::string> _any_file(const std::string &path)
{
    if (file_exists(path))
        return std::vector{path};
    return std::vector<std::string>{};
}

template <typename... Args>
std::vector<std::string> _any_file(const std::string &path, Args &&... args)
{
    if (file_exists(path)) {
        // Concatenate vectors
        std::vector v{path};
        std::vector next{_any_file(std::forward<Args>(args)...)};
        v.insert(v.end(), next.begin(), next.end());
        return v;
    }
    return _any_file(std::forward<Args>(args)...);
}

// Will return _all_ paths which have been found
template <typename... Args>
std::vector<std::string> any_file(Args &&... args)
{
    return _any_file(std::forward<Args>(args)...);
}

// typename CT: Chrono time point
class time_point
{
    std::time_t m_time_t;

    // localtime is thread unsafe, so we lock.
    static std::mutex mtx;

public:
    time_point(std::time_t t)
        : m_time_t(t)
    {
    }

    time_point(const time_point &tp)
        : m_time_t(tp.m_time_t)
    {
    }

    void operator=(const time_point &tp)
    {
        m_time_t = tp.m_time_t;
    }

    std::string to_string() const
    {
        std::lock_guard<std::mutex> guard(mtx);
        std::string s(26, '\0');
        std::strftime(&s[0], 25, "%Y-%m-%d %H:%M:%S %Z",
                      std::localtime(&m_time_t));
        return s;
    }
};

// Instantiate time_point::mtx inline.
inline std::mutex time_point::mtx;

class system_time
{
public:
    static time_point now()
    {
        return time_point(std::time(nullptr));
    }
};

class guard
{
    std::mutex mtx;

public:
    template <typename Function>
    auto operator()(Function f)
    {
        std::lock_guard<std::mutex> g(mtx);
        return f();
    }
};

}; // namespace util

}; // namespace ns

#endif
