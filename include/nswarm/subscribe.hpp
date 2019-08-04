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

    // Include overloads that sync m_event
    LAYER_CONSTRUCTOR(subscription, event)

    const std::string &event() const
    {
        return m_event;
    }

private:
    template <action::type>
    friend subscription make_subscription(const std::string &);
};

template <action::type action_t>
subscription make_subscription(const std::string &event)
{
    ns::json js{{"event", event}};
    auto impl =
        subscription(net::header(message::type::implement, 0, error::type::none,
                                 action_t, js.dump().size()),
                     js);
    impl.m_event = impl.get_json().at("event");
    return impl;
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

subscription make_subscription_error(const std::string &event,
                                     const std::string &error)
{
    // Create a normal impl
    auto impl = make_subscription<action::type::response>(event);

    // Then, set the error field in json...
    auto js = impl.get_json();
    js["error"] = error;
    auto size = js.dump().size(); // acquiire new size

    // And update the header with error::type::set and the new json
    impl.update(net::header(impl.get_type(), impl.head().args(),
                            error::type::set, impl.get_action(), size),
                std::move(js));

    return impl;
}

}; // namespace net
}; // namespace ns

#endif // NS_SUBSCRIBE_HPP
