/**
 * \prject nswarm
 * \file dispatcher.hpp
 * \brief Task dispatcher objects and tools. The task dispatcher
 * should only be used in servers from the api user to the node.
 *
 * When we receive a particular task, we will assign a specific
 * callback to it, so that we can trigger it when we receive
 * a response from downstream. This mechanism can be used to
 * reply to the task origin.
 *
 * Copyright (c) 2019 Kevin Morris
 * All Rights Reserved.
 **/
#ifndef NS_DISPATCHER_HPP
#define NS_DISPATCHER_HPP

#include <functional>
#include <memory>
#include <mutex>
#include <nswarm/data.hpp>
#include <nswarm/task.hpp>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>

namespace ns
{

template <typename T>
using async_task_function =
    std::function<void(std::shared_ptr<T>, net::json_message)>;

template <typename T>
class task_dispatcher
{
public: // class type aliases
    using task_function = async_task_function<T>;
    using tuple = std::tuple<std::string, task_function>;

private: // private members
    std::unordered_map<std::string, task_function>
        m_tasks; // Standard task flow

    std::mutex m_mutex;

public:
    task_dispatcher() = default;

    task_dispatcher(task_dispatcher &&dp)
        : m_tasks(std::move(dp.m_tasks))
    {
    }

    void operator=(task_dispatcher &&dp)
    {
        m_tasks = std::move(dp.m_tasks);
    }

    void push(const std::string &task_id, task_function fn)
    {
        std::lock_guard<std::mutex> g(m_mutex);
        if (unsafe_exists(task_id))
            throw std::out_of_range("key already exists");
        m_tasks.emplace(task_id, fn);
    }

    // Thread-safe combination of exists/pop. If the item
    // exists, it will pop it out and return it here while
    // under a lock.
    std::optional<tuple> pop(const std::string &task_id)
    {
        std::lock_guard<std::mutex> g(m_mutex);
        if (!unsafe_exists(task_id))
            return std::nullopt;
        return pop_tuple(task_id);
    }

    bool exists(const std::string &task_id) const
    {
        std::lock_guard<std::mutex> g(m_mutex);
        return unsafe_exists(task_id);
    }

private:
    bool unsafe_exists(const std::string &task_id)
    {
        return m_tasks.find(task_id) != m_tasks.end();
    }

    tuple pop_tuple(const std::string &task_id)
    {
        auto task = m_tasks.find(task_id);
        auto tv = std::move(task->second);
        m_tasks.erase(task);
        return std::make_tuple(task_id, std::move(tv));
    }

protected:
    // Friends
    template <typename U>
    friend std::string make_task_id(const task_dispatcher<U> &dispatcher)
    {
        std::string task_id; // set this to a random task_id
        while (dispatcher.safe_exists(task_id)) {
            // set new task_id
        }
        return task_id;
    }
};

}; // namespace ns

#endif
