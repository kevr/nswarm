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
#include <random>
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
        if (m_json.find("method") != m_json.end())
            throw std::logic_error(
                "this task has already been set as an event");
        m_json["event"] = event;
        if (head().args() != task::type::event)
            update_args(task::type::event);
        update(std::move(m_json));
    }

    void set_method(const std::string &method)
    {
        if (m_json.find("event") != m_json.end())
            throw std::logic_error(
                "this task has already been set as an event");
        m_json["method"] = method;
        if (head().args() != task::type::call)
            update_args(task::type::call);
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

    template <action::type, typename... Args>
    friend net::task make_task(Args &&...);
};

template <action::type action_t, typename... Args>
net::task make_task(Args &&... args)
{
    auto t = net::task(std::forward<Args>(args)...);
    if (t.get_action() != action_t)
        t.update_action(action_t);
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

// Only responses can be errors. It doesn't make sense to request an error.
inline net::task make_task_error(const std::string &task_id,
                                 const std::string &error_msg)
{
    auto t = net::make_task_response(task_id);
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
            auto task = net::task(uuid);
            task.update_args(decltype(t)::type);
            return task;
        });
        t.on_response(on_resp);
        const auto task_id = t.task_id();
        m_tasks.emplace(task_id, std::move(t));
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

template <template <typename> typename Container>
std::string make_unique_task_id(const Container<net::task> &c)
{
    std::string task_id;

    static const auto &chrs = "0123456789abcdef";
    static std::random_device rd;
    static std::uniform_int_distribution<std::string::size_type> pick(
        0, sizeof(chrs) - 2);

    std::set<std::string> s;
    for (auto &e : c)
        s.emplace(e.task_id());

    auto make_string = [](auto length) {
        std::string str;
        str.reserve(length);
        while (--length)
            str.push_back(chrs[pick(rd)]);
        return str;
    };

    task_id = make_string(16);
    while (s.find(task_id) != s.end())
        task_id = make_string(16);

    return task_id;
}

template <template <typename, typename> typename Container>
std::string make_unique_task_id(const Container<std::string, net::task> &c)
{
    std::string task_id;

    static const auto &chrs = "0123456789abcdef";
    static std::random_device rd;
    static std::uniform_int_distribution<std::string::size_type> pick(
        0, sizeof(chrs) - 2);

    auto make_string = [](auto length) {
        std::string str;
        str.reserve(length);
        while (--length)
            str.push_back(chrs[pick(rd)]);
        return str;
    };

    task_id = make_string(16);
    while (c.find(task_id) != c.end())
        task_id = make_string(16);
    return task_id;
}

}; // namespace net

}; // namespace ns

#endif // NS_TASK_HPP
