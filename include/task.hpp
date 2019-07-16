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
 * task_value: Flags field of a task data message.
 *     call (0): Call a provided method
 *     emit (1): Emit a subscribed to event
 **/
namespace value
{
enum task_value : uint16_t { call = 1, emit = 2 };
};

namespace task_type
{

struct call {
    static constexpr value::task_value value = value::task_value::call;
};

struct emit {
    static constexpr value::task_value value = value::task_value::emit;
};

using variant = std::variant<call, emit>;

inline variant deduce(const value::task_value t)
{
    switch (t) {
    case value::task_value::call:
        return call();
    case value::task_value::emit:
        return emit();
    }
    throw std::invalid_argument("Invalid task type: " + std::to_string(t));
}

}; // namespace task_type

//
// typename T: task_type
// typename A: action_type
template <typename T, typename A>
class task : public data
{
    // private members
    std::string m_task_id;

public: // static functions
    static constexpr uint16_t type = T::value;
    static constexpr value::action_value action = A::value;

    static task<T, A> deserialize(const std::string &data)
    {
        return task<T, A>(json::parse(data));
    }

    static std::string serialize(const task<T, A> &t)
    {
        return t.get_string();
    }

public: // public functions
    task() = default;

    // We need to fix this
    task(const json &js)
        : task(serialize_header(value::data_value::task, T::value, A::value, 0),
               js)
    {
        m_task_id = js["task_id"];
        set_size(get_string().size());
    }

    // Unfortunately, we have to overload these constructors and assignments to
    // include local m_task_id
    task(const task &t)
        : data(t)
    {
        m_task_id = t.m_task_id;
    }

    void operator=(const task &t)
    {
        data::operator=(t);
        m_task_id = t.m_task_id;
    }

    task(task &&t)
        : data(std::move(t))
    {
        m_task_id = std::move(t.m_task_id);
    }

    void operator=(task &&t)
    {
        data::operator=(std::move(t));
        m_task_id = std::move(t.m_task_id);
    }

    virtual ~task() = default;

    operator bool() const
    {
        return m_task_id.size() > 0;
    }

    const std::string &task_id() const
    {
        return m_task_id;
    }
}; // namespace tasks

template <typename T>
using task_request = task<T, action_type::request>;

template <typename T>
using task_response = task<T, action_type::response>;

}; // namespace ns

#endif // NS_TASK_HPP
