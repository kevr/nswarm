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

#include <nswarm/data.hpp>
#include <nswarm/response.hpp>

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace ns
{

namespace net
{

class task : public json_message, protected responsive<task>
{
protected:
    std::string m_task_id;

public:
    enum type { call = 0x0, event = 0x1, bad };

    struct tag {
        struct call {
            using object = task;
            static constexpr task::type type = task::type::call;
            static constexpr const char *const human = "task::type::call";
        };
        struct event {
            using object = task;
            static constexpr task::type type = task::type::event;
            static constexpr const char *const human = "task::type::event";
        };
        struct bad {
            using object = task;
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

    task(const std::string &task_id)
        : m_task_id(task_id)
    {
        update(ns::json{{"task_id", task_id}});
        update(net::header(message::type::task, 0, error::type::none,
                           action::type::request, m_data.size()));
    }

    // We need to specifically provide more extended
    // constructors to carry along the on_response
    // callback if provided
    task(const task &t)
        : json_message(t)
    {
        m_task_id = t.m_task_id;
        on_response(t.m_response_f);
    }

    task(task &&t)
        : json_message(std::move(t))
    {
        m_task_id = std::move(t.m_task_id);
        on_response(std::move(t.m_response_f));
    }

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

    void operator=(task t)
    {
        json_message::operator=(std::move(t));
        m_task_id = std::move(t.m_task_id);
        on_response(std::move(t.m_response_f));
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

    // Make these publicly accessible
    using responsive<task>::on_response;
    using responsive<task>::call_response;

    void respond(net::task t) noexcept
    {
        return call_response(std::move(t));
    }

    void set_event(const std::string &event)
    {
        m_json["event"] = event;
        update(std::move(m_json));
    }

    void set_method(const std::string &method)
    {
        m_json["method"] = method;
        update(std::move(m_json));
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

    template <task::type, action::type, typename... Args>
    friend net::task make_task(Args &&...);
};

template <task::type task_t, action::type action_t, typename... Args>
net::task make_task(Args &&... args)
{
    auto t = net::task(std::forward<Args>(args)...);
    t.update_args(task_t);
    t.update_action(action_t);
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

// Only responses can be errors. It doesn't make sense to request an error.
template <task::type task_t>
net::task make_task_error(const std::string &task_id,
                          const std::string &error_msg)
{
    auto t = net::make_task_response<task_t>(task_id);
    t.update_error(error::type::set, error_msg);
    return t;
}

// Task manager class.
// When we receive a task request, we can store the task
// with a lambda that takes a copy of the request origin's connection.
//
// Ex.
//
// match(task::deduce(msg.head().args()),
//   [&](auto task_t) {
//       auto t = make_task_request<decltype(task_t)>
//       dispatcher.create(t, [client](ns::task resp) {
//           if(resp.has_error()) {
//               loge("Response has an error: ", resp.get_json().at("error"));
//           }
//           client->send(std::move(resp));
//       });
//   });
//
class task_dispatcher
{
    std::unordered_map<std::string, net::task> m_tasks;

public:
    task_dispatcher() = default;

    void create(net::task task_, async_response_function<net::task> on_resp)
    {
        const std::string uuid(task_.task_id());
        auto t = match(net::task::deduce(task_.get_task_type()), [=](auto t) {
            return net::make_task_request<decltype(t)::type>(uuid);
        });
        t.on_response(on_resp);
        auto task_id = t.task_id();
        m_tasks.emplace(t.task_id(), std::move(t));
    }

    /**
     * \param t A task response.
     * \return The popped task
     * This function throws if the given task's task_id is not
     * found in the internal task map.
     **/
    net::task respond(net::task response)
    {
        if (response.get_action() != action::type::response)
            throw std::invalid_argument("task request supplied to response(t)");

        auto task_i = m_tasks.find(response.task_id());
        if (task_i == m_tasks.end())
            throw std::out_of_range(response.task_id() +
                                    " is an invalid task_id");

        auto &[task_id, task] = *task_i;
        task.call_response(std::move(response)); // Callback

        auto x = std::move(task); // Move task out of the table
        m_tasks.erase(task_i);    // Task is done, erase the entry
        return std::move(x);      // Return the task structure to the user
    }
};

}; // namespace net

}; // namespace ns

#endif // NS_TASK_HPP
