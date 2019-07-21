/**
 * \prject nswarm
 * \file memory.hpp
 * \brief Process memory sensory data.
 *
 * Copyright (c) 2019 Kevin Morris
 * All Rights Reserved.
 **/
#ifndef NS_MEMORY_HPP
#define NS_MEMORY_HPP

#include <nswarm/logging.hpp>
#include "process.hpp"
#include "sensor.hpp"
#include <atomic>
#include <mutex>
#include <string>
#include <thread>

namespace ns
{
class memory_sensor : public sensor<uint64_t>
{
public:
    using sensor::sensor;

protected:
    virtual void work()
    {
        trace();
        m_bytes_in_use = bytes_in_use();
    }

    virtual void locked()
    {
        trace();
        m_usage = m_bytes_in_use;
    }

    virtual uint64_t get()
    {
        trace();
        return m_usage;
    }

private:
    // Keep two separate variables, a presentable usage
    // that users can .get_value() of, and a cache of
    // the current bytes in use. the m_usage will only
    // be assigned in locked(), after the work has been
    // done to collect the data from the system.
    uint64_t m_usage = 0;
    uint64_t m_bytes_in_use = 0; // result from work

    set_log_address;
};

}; // namespace ns

#endif
