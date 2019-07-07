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

#include "logging.hpp"
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
        m_usage = bytes_in_use();
    }

    virtual uint64_t get()
    {
        trace();
        return m_usage;
    }

private:
    uint64_t m_usage = 0; // actually in kb right now.

    set_log_address;
};

}; // namespace ns

#endif
