#ifndef NS_SUBSCRIBE_HPP
#define NS_SUBSCRIBE_HPP

#include <map>
#include <nswarm/data.hpp>
#include <nswarm/task.hpp>
#include <set>

namespace ns
{

namespace net
{

// Represents net::message::subscribe data.
class subscription : public json_message
{
    std::string m_event;

public:
    using json_message::json_message;

    subscription(const std::string &event)
        : m_event(event)
    {
        update(ns::json{{"event", event}});
        update(net::header(message::type::subscribe, 0, error::type::none,
                           action::type::request, m_data.size()));
    }

    // Include overloads that sync m_event
    LAYER_CONSTRUCTOR(subscription, event)

    const std::string &event() const
    {
        return m_event;
    }

private:
    template <action::type, typename... Args>
    friend subscription make_subscription(Args &&...);
};

template <action::type action_t, typename... Args>
subscription make_subscription(Args &&... args)
{
    auto sub = subscription(std::forward<Args>(args)...);
    if (action_t != sub.get_action())
        sub.update_action(action_t);
    return sub;
}

template <typename... Args>
subscription make_subscription_request(Args &&... args)
{
    return make_subscription<action::type::request>(
        std::forward<Args>(args)...);
}

template <typename... Args>
subscription make_subscription_response(Args &&... args)
{
    return make_subscription<action::type::response>(
        std::forward<Args>(args)...);
}

inline subscription make_subscription_error(const std::string &event,
                                            const std::string &error)
{
    // Create a normal impl
    auto sub = make_subscription<action::type::response>(event);
    sub.update_error(error::type::set, error);
    return sub;
}

}; // namespace net

// A class that deals with subscriptions
// typename T: Connection type
template <typename T>
class subscription_manager
{
    std::map<std::string, std::set<std::shared_ptr<T>>> m_subscriptions;

public:
    subscription_manager() = default;

    void subscribe(const std::string &event, std::shared_ptr<T> connection)
    {
        // Subscribe a connection to a specific event
        m_subscriptions[event].emplace(connection);
    }

    void remove(const std::shared_ptr<T> connection)
    {
        // Remove this connection from all methods in the subscription table
        for (auto &kv : m_subscriptions) {
            auto &s = kv.second;
            auto iter = s.find(connection);
            if (iter != s.end())
                s.erase(iter);
        }
    }

    void broadcast(const std::string &event)
    {
        auto iter = m_subscriptions.find(event);
        if (iter != m_subscriptions.end()) {
            for (auto &connection : iter->second)
                send_event_task(connection, "taskUUID", event);
            logi("broadcasted event: ", event);
        } else {
            loge("broadcast requested for event '", event,
                 "' but no associated connections available");
        }
    }

protected:
    void send_event_task(std::shared_ptr<T> &connection,
                         const std::string &task_id, const std::string &event)
    {
        auto task = net::make_task_request<net::task::type::event>(task_id);
        task.set_event(event);
        connection->send(std::move(task));
    }
};

}; // namespace ns

#endif // NS_SUBSCRIBE_HPP
