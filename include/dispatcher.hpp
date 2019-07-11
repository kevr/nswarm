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

#include "data.hpp"
#include "task.hpp"
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>

namespace ns
{

template <typename T>
using async_task_function = std::function<void(std::shared_ptr<T>, data)>;

template <typename T>
class task_dispatcher
{
public:
    void push(const std::string &task_id, async_task_function<T> fn)
    {
        std::lock_guard<std::mutex> g(lock());
        if (unsafe_exists(task_id))
            throw std::out_of_range("key already exists");
        m_tasks.emplace(task_id, fn);
    }

    // Thread-safe combination of exists/pop. If the item
    // exists, it will pop it out and return it here while
    // under a lock.
    std::optional<std::tuple<std::string, async_task_function<T>>>
    pop(const std::string &task_id)
    {
        std::lock_guard<std::mutex> g(lock());
        if (!unsafe_exists(task_id))
            return std::nullopt;
        return pop_tuple(task_id);
    }

    bool exists(const std::string &task_id) const
    {
        std::lock_guard<std::mutex> g(lock());
        return unsafe_exists(task_id);
    }

private:
    std::lock_guard<std::mutex> lock()
    {
        return std::lock_guard<std::mutex>(m_mutex);
    }

    bool unsafe_exists(const std::string &task_id)
    {
        return m_tasks.find(task_id) != m_tasks.end();
    }

    std::tuple<std::string, async_task_function<T>>
    pop_tuple(const std::string &task_id)
    {
        auto task = m_tasks.at(task_id);
        m_tasks.erase(m_tasks.find(task_id));
        return std::make_tuple(task_id, task);
    }

private:
    std::unordered_map<std::string, async_task_function<T>> m_tasks;
    std::mutex m_mutex;
};

template <typename T>
std::string make_task_id(const task_dispatcher<T> &dispatcher)
{
    std::string task_id; // set this to a random task_id
    while (dispatcher.safe_exists(task_id)) {
        // set new task_id
    }
    return task_id;
}

}; // namespace ns

#endif
