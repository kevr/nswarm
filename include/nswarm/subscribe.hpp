#ifndef NS_SUBSCRIBE_HPP
#define NS_SUBSCRIBE_HPP

#include <nswarm/data.hpp>

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
}; // namespace ns

#endif // NS_SUBSCRIBE_HPP
