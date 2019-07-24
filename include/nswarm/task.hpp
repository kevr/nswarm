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

#include <cstdint>
#include <nswarm/auth.hpp>
#include <nswarm/data.hpp>
#include <string>

namespace ns
{

namespace net
{

class task : public json_message
{
protected:
    std::string m_task_id;

public:
    enum type { call = 0x0, event = 0x1, bad };

    struct tag {
        struct call {
            static constexpr task::type type = task::type::call;
            static constexpr const char *const human = "task::type::call";
        };
        struct event {
            static constexpr task::type type = task::type::event;
            static constexpr const char *const human = "task::type::event";
        };
        struct bad {
            static constexpr task::type type = task::type::bad;
            static constexpr const char *const human = "task::type::bad";
        };
    };

    using variant = std::variant<tag::call, tag::event, tag::bad>;

    static constexpr variant deduce(task::type t)
    {
        switch (t) {
        case task::type::call:
            return tag::call{};
        case task::type::event:
            return tag::event{};
        case task::type::bad:
            return tag::bad{};
        }
        return tag::bad{};
    }
    static constexpr variant deduce(uint16_t t)
    {
        return deduce(static_cast<task::type>(t));
    }

public:
    using json_message::json_message;

    task(const task &t)
        : json_message(t)
    {
        m_task_id = t.m_task_id;
    }

    task(task &&t)
        : json_message(std::move(t))
    {
        m_task_id = std::move(t.m_task_id);
    }

    void operator=(task t)
    {
        json_message::operator=(std::move(t));
        m_task_id = std::move(t.m_task_id);
    }

    // These should be provided for all L2 derivatives that
    // have custom member variables.
    task(const json_message &m)
        : json_message(m)
    {
        m_task_id = get_json().at("task_id");
    }

    task(json_message &&m)
        : json_message(std::move(m))
    {
        m_task_id = get_json().at("task_id");
    }

    void operator=(json_message m)
    {
        json_message::operator=(std::move(m));
        m_task_id = get_json().at("task_id");
    }

    const std::string &task_id() const
    {
        return m_task_id;
    }

private:
    template <action::type>
    friend task make_task(const std::string &, ns::json);

    template <action::type>
    friend task make_task(const std::string &);
}; // namespace net

template <action::type action_t>
net::task make_task(const std::string &task_id, ns::json js)
{
    if (task_id.size() == 0)
        throw std::out_of_range("task_id cannot be empty");

    js["task_id"] = task_id;

    auto size = js.dump().size();
    net::task t(
        net::header(message::type::task, error::type::none, 0, action_t, size),
        std::move(js));
    t.m_task_id = task_id;

    return t;
}

template <action::type action_t>
net::task make_task(const std::string &task_id)
{
    if (task_id.size() == 0)
        throw std::out_of_range("task_id cannot be empty");

    ns::json js;
    js["task_id"] = task_id;

    auto size = js.dump().size();
    net::task t(
        net::header(message::type::task, error::type::none, 0, action_t, size),
        std::move(js));
    t.m_task_id = task_id;

    return t;
}

template <typename... Args>
net::task make_task_request(Args &&... args)
{
    return make_task<action::type::request>(std::forward<Args>(args)...);
}

template <typename... Args>
net::task make_task_response(Args &&... args)
{
    return make_task<action::type::response>(std::forward<Args>(args)...);
}

inline net::task make_task_error(const std::string &task_id)
{
    // Use friendly function, then update the header with error bit
    auto t = make_task<action::type::response>(task_id);
    t.update(net::header(t.get_type(), t.head().args(), error::type::set,
                         t.get_action(), t.head().size()));
    return t;
}

// Only responses can be errors. It doesn't make sense to request an error.
inline net::task make_task_error(const std::string &task_id,
                                 const std::string &error_msg)
{
    auto t = make_task_error(task_id);
    auto js = t.get_json();
    js["error"] = error_msg;
    t.update(js);
    return t;
}

}; // namespace net

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
    using data::data;

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
