/**
 * \prject nswarm
 * \file task.hpp
 * \brief Structures representing a task of nswarm.
 *
 * Copyright (c) 2019 Kevin Morris
 * All Rights Reserved.
 **/
#ifndef NS_TASK_HPP
#define NS_TASK_HPP

#include "auth.hpp"
#include "data.hpp"
#include <cstdint>
#include <string>

namespace ns
{

/**
 * task_type: Flags field of a task data message.
 *     call (0): Call a provided method
 *     emit (1): Emit a subscribed to event
 **/
enum task_type : uint16_t { call, emit };

namespace tasks
{

struct task_tag {
};

struct call : public task_tag {
    static constexpr task_type type_value = task_type::call;
};
struct emit : public task_tag {
    static constexpr task_type type_value = task_type::emit;
};

class basic_task
{
public:
    basic_task() = default;

    basic_task(const std::string &task_id)
        : m_task_id(task_id)
    {
    }

    basic_task(const basic_task &t)
        : m_task_id(t.m_task_id)
    {
    }

    void operator=(const basic_task &t)
    {
        m_task_id = t.m_task_id;
    }

    basic_task(basic_task &&t)
        : m_task_id(std::move(t.m_task_id))
    {
    }

    void operator=(basic_task &&t)
    {
        m_task_id = std::move(t.m_task_id);
    }

    virtual ~basic_task() = default;

    const std::string &task_id() const
    {
        return m_task_id;
    }

private:
    std::string m_task_id;
};

template <typename TaskType>
class task : public basic_task
{
    // We don't use this
};

// Define custom setup functions for json
template <>
class task<call> : public basic_task
{
public:
    using basic_task::basic_task;
    virtual ~task() = default;

    virtual uint16_t type() const
    {
        return call::type_value;
    }

    std::string serialize(json js)
    {
        if (js.find("method") == js.end())
            throw std::invalid_argument("no method provided in json");

        if (js.find("task_id") == js.end())
            js["task_id"] = task_id();

        if (js.find("data") == js.end())
            js["data"] = nullptr;

        return js.dump();
    }
};

template <>
class task<emit> : public basic_task
{
public:
    using basic_task::basic_task;
    virtual ~task() = default;

    virtual uint16_t type() const
    {
        return emit::type_value;
    }

    std::string serialize(json js)
    {
        if (js.find("event") == js.end())
            throw std::invalid_argument("no event provided in json");

        if (js.find("task_id") == js.end()) {
            if (task_id().size() == 0)
                throw std::out_of_range("no task_id to serialize");
            js["task_id"] = task_id();
        }

        return js.dump();
    }

    void deserialize(const std::string &data)
    {
        auto js = json::parse(data);
        *this = task<emit>(js["task_id"]);
    }
};

}; // namespace tasks

}; // namespace ns

#endif // NS_TASK_HPP
