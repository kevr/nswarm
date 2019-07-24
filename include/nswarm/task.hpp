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
#include <map>
#include <nswarm/auth.hpp>
#include <nswarm/data.hpp>
#include <set>
#include <string>
#include <vector>

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
        m_response_f = t.m_response_f;
        m_task_id = t.m_task_id;
    }

    task(task &&t)
        : json_message(std::move(t))
    {
        m_response_f = std::move(t.m_response_f);
        m_task_id = std::move(t.m_task_id);
    }

    void operator=(task t)
    {
        json_message::operator=(std::move(t));
        m_response_f = std::move(t.m_response_f);
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

    const task::type get_task_type() const
    {
        return static_cast<task::type>(head().args());
    }

    void on_response(std::function<void(net::task)> f)
    {
        m_response_f = f;
    }

    void respond(net::task t) noexcept
    {
        if (m_response_f)
            m_response_f(std::move(t));
    }

private:
    // This task callback function should capture its necessary
    // connections by copies of their shared_ptrs when registering
    // on_response. Typically, you would have an upstream connection
    // in this callback to trigger propagation.
    //
    // Ex.
    //
    // on_response([=](net::task t) {
    //     upstream->send(std::move(t));
    // });
    //
    std::function<void(net::task)> m_response_f;

    template <task::type, action::type>
    friend net::task make_task(const std::string &, ns::json);

    template <task::type, action::type>
    friend net::task make_task(const std::string &);
};

template <task::type task_t, action::type action_t>
net::task make_task(const std::string &task_id, ns::json js)
{
    if (task_id.size() == 0)
        throw std::out_of_range("task_id cannot be empty");

    js["task_id"] = task_id;

    auto size = js.dump().size();
    net::task t(net::header(message::type::task, task_t, error::type::none,
                            action_t, size),
                std::move(js));
    t.m_task_id = task_id;

    return t;
}

template <task::type task_t, action::type action_t>
net::task make_task(const std::string &task_id)
{
    if (task_id.size() == 0)
        throw std::out_of_range("task_id cannot be empty");

    ns::json js;
    js["task_id"] = task_id;

    auto size = js.dump().size();
    net::task t(net::header(message::type::task, task_t, error::type::none,
                            action_t, size),
                std::move(js));
    t.m_task_id = task_id;

    return t;
}

template <task::type task_t, typename... Args>
net::task make_task_request(Args &&... args)
{
    return make_task<task_t, action::type::request>(
        std::forward<Args>(args)...);
}

template <task::type task_t, typename... Args>
net::task make_task_response(Args &&... args)
{
    return make_task<task_t, action::type::response>(
        std::forward<Args>(args)...);
}

template <task::type task_t>
net::task make_task_error(const std::string &task_id)
{
    // Use friendly function, then update the header with error bit
    auto t = make_task<task_t, action::type::response>(task_id);
    t.update(net::header(t.get_type(), t.head().args(), error::type::set,
                         t.get_action(), t.head().size()));
    return t;
}

// Only responses can be errors. It doesn't make sense to request an error.
template <task::type task_t>
net::task make_task_error(const std::string &task_id,
                          const std::string &error_msg)
{
    auto t = make_task_error<task_t>(task_id);
    auto js = t.get_json();
    js["error"] = error_msg;
    t.update(js);
    return t;
}

// Task manager class.
class task_dispatcher
{
    std::unordered_map<std::string, net::task> m_tasks;

public:
    task_dispatcher() = default;

    net::task create(task::type task_t, std::function<void(net::task)> on_resp)
    {
        const std::string uuid("taskUUID");
        auto t = match(task::deduce(task_t), [=](auto t) {
            return make_task_request<decltype(t)::type>(uuid);
        });
        t.on_response(on_resp);
        auto task_id = t.task_id();
        m_tasks.emplace(t.task_id(), std::move(t));
        return t;
    }

    /**
     * \param t A task response.
     * \return The popped task
     * This function throws if the given task's task_id is not
     * found in the internal task map.
     **/
    net::task respond(net::task t)
    {
        if (t.get_action() != action::type::response)
            throw std::invalid_argument("task request supplied to response(t)");

        auto task_i = m_tasks.find(t.task_id());
        if (task_i == m_tasks.end())
            throw std::out_of_range(t.task_id() + " is an invalid task_id");

        auto [k, v] = *task_i;

        v.respond(std::move(t)); // We might want exceptions here for ctrl
        m_tasks.erase(task_i);   // Task is done.

        return std::move(v); // Return the task structure to the user
    }
};

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
