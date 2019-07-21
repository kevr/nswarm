/**
 * \project nswarm
 * \file logging.hpp
 *
 * Copyright (c) 2019 Kevin Morris
 * All Rights Reserved.
 **/
#ifndef NS_LOGGING_HPP
#define NS_LOGGING_HPP

#include <iomanip>
#include <iostream>
#include <libgen.h>
#include <map>
#include <mutex>
#include <sstream>

namespace ns
{

// Color
namespace color
{
struct normal {
    static constexpr const char *const value = "\033[0m";

    virtual const char *const get() const
    {
        return value;
    }
};

struct green : public normal {
    static constexpr const char *const value = "\033[1;32m";

    virtual const char *const get() const
    {
        return value;
    }
};

struct red : public normal {
    static constexpr const char *const value = "\033[1;31m";

    virtual const char *const get() const
    {
        return value;
    }
};

struct yellow : public normal {
    static constexpr const char *const value = "\033[1;33m";

    virtual const char *const get() const
    {
        return value;
    }
};
}; // namespace color

inline std::stringstream &operator<<(std::stringstream &ss,
                                     const color::normal &n)
{
    ss << n.get();
    return ss;
}

namespace detail
{

/**
 * \brief This class uses std::cout/cerr to log. It redirects
 *
 **/
class logstream
{
public:
    static logstream &instance()
    {
        static logstream i;
        return i;
    }

    logstream() = default;
    ~logstream() = default;

    static void set_debug(bool enabled)
    {
        instance().m_debug = enabled;
    }

    static void set_trace(bool enabled)
    {
        instance().m_trace = enabled;
    }

    template <typename... Args>
    void out(Args &&... args)
    {
        // New stringstream for this recursive call. We use a standalone ss
        // in order to remain thread-safe.
        std::stringstream ss;
        f_out(ss, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void info(Args &&... args)
    {
        out("[ ", color::green::value, "INFO", color::normal::value, " ] ",
            std::forward<Args>(args)...);
    }

    template <typename... Args>
    void error(Args &&... args)
    {
        out("[  ", color::red::value, "ERR", color::normal::value, " ] ",
            std::forward<Args>(args)...);
    }

    template <typename... Args>
    void debug(Args &&... args)
    {
        if (m_debug) {
            out("[  ", color::yellow::value, "DBG", color::normal::value, " ] ",
                std::forward<Args>(args)...);
        }
    }

    template <typename... Args>
    void trace(Args &&... args)
    {
        if (m_trace) {
            out("[ ", color::yellow::value, "TRCE", color::normal::value, " ] ",
                std::forward<Args>(args)...);
        }
    }

    bool redirect(const std::string &path)
    {
        FILE *chk = fopen(path.c_str(), "a+");
        if (!chk)
            return false;
        fclose(chk);
        return freopen(path.c_str(), "a+", stdout) != nullptr;
    }

private:
    template <typename T, typename... Args>
    void f_out(std::stringstream &ss, T &&arg, Args &&... args)
    {
        // Continue f_out chain
        ss << arg;
        f_out(ss, std::forward<Args>(args)...);
    }

    template <typename T>
    void f_out(std::stringstream &ss, T &&arg)
    {
        // Last f_out call, single argument left. Close out and call cout
        // a single time, as to avoid interleaving on console.
        ss << arg << std::endl;
        std::cout << ss.str(); // Thread-safe
    }

private:
    static inline bool m_debug = false;
    static inline bool m_trace = false;

public: // public accessor for m_debug
    static inline bool has_debug_logging()
    {
        return m_debug;
    }

    static inline bool has_trace_logging()
    {
        return m_trace;
    }
};

}; // namespace detail

static inline bool has_debug_logging()
{
    return ns::detail::logstream::has_debug_logging();
}

static inline bool has_trace_logging()
{
    return ns::detail::logstream::has_trace_logging();
}

// Create a global reference to our logstream
static detail::logstream &cout = detail::logstream::instance();

}; // namespace ns

static inline const std::string p_secret_log_addr = "";

#define logi(...)                                                              \
    ns::cout.info(::basename((char *)__FILE__), '(', __LINE__, ") ",           \
                  p_secret_log_addr.size() ? "[" + p_secret_log_addr + "] "    \
                                           : "",                               \
                  __func__, "(): ", __VA_ARGS__);

#define logd(...)                                                              \
    ns::cout.debug(::basename((char *)__FILE__), '(', __LINE__, ") ",          \
                   p_secret_log_addr.size() ? "[" + p_secret_log_addr + "] "   \
                                            : "",                              \
                   __func__, "(): ", __VA_ARGS__);

#define loge(...)                                                              \
    ns::cout.error(::basename((char *)__FILE__), '(', __LINE__, ") ",          \
                   p_secret_log_addr.size() ? "[" + p_secret_log_addr + "] "   \
                                            : "",                              \
                   __func__, "(): ", __VA_ARGS__);

const std::string hexify(unsigned long value)
{
    std::stringstream ss;
    ss << std::showbase << std::internal << std::setfill('0') << std::hex
       << std::setw(4) << value;
    return ss.str();
}

// set_log_address;
#define set_log_address                                                        \
    const std::string p_secret_log_addr = hexify((unsigned long)&*this)

#define set_mutable_log_address                                                \
    std::string p_secret_log_addr = hexify((unsigned long)&*this)

namespace ns
{
static inline void set_debug_logging(bool enabled)
{
    ns::detail::logstream::instance().set_debug(enabled);
    if (enabled)
        logd("enabled debug logging");
}
static inline void set_trace_logging(bool enabled)
{
    set_debug_logging(enabled);
    ns::detail::logstream::instance().set_trace(enabled);
    if (enabled)
        logd("enabled trace logging");
}
}; // namespace ns

class atexit_func
{
public:
    atexit_func(const std::string &fname, std::size_t line,
                const std::string &log_addr, const std::string &func)
    {
        // Do as little work as possible
        if (ns::cout.has_trace_logging()) {
            pre = fname + "(" + std::to_string(line) + ") " + log_addr + func +
                  "(): ";
            ns::cout.trace(pre, "START");
        }
    }

    ~atexit_func()
    {
        ns::cout.trace(pre, "END");
    }

private:
    std::string pre;
};

#define trace()                                                                \
    atexit_func secret_atexit(                                                 \
        basename((char *)__FILE__), __LINE__,                                  \
        p_secret_log_addr.size() ? "[" + p_secret_log_addr + "] " : "",        \
        __func__)

#define tracemove() logd("MOVE");
#define tracecopy() logd("COPY");
#define tracevalue() logd("VALUE");

#define tracef(val) logd(val)

#endif
