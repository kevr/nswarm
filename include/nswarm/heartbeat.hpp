#ifndef NS_HEARTBEAT_HPP
#define NS_HEARTBEAT_HPP

#include <iostream>
#include <nswarm/data.hpp>

namespace ns
{

namespace net
{

class heartbeat : public json_message
{
public:
    using json_message::json_message;

    EASY_LAYER_CONSTRUCTOR(heartbeat)

private:
    template <action::type>
    friend heartbeat make_heartbeat();
};

template <action::type action_t>
heartbeat make_heartbeat()
{
    heartbeat hb(net::header(message::type::heartbeat, 0, error::type::none,
                             action_t, 0));
    return hb;
}

template <typename... Args>
heartbeat make_heartbeat_request(Args &&... args)
{
    return make_heartbeat<action::type::request>(std::forward<Args>(args)...);
}

template <typename... Args>
heartbeat make_heartbeat_response(Args &&... args)
{
    return make_heartbeat<action::type::response>(std::forward<Args>(args)...);
}

// Inline to avoid ODR violation.
inline heartbeat make_heartbeat_error(const std::string &error)
{
    // Create a normal impl
    auto hb = make_heartbeat<action::type::response>();
    hb.update_error(error::type::set, error);
    return hb;
}

}; // namespace net
}; // namespace ns

#endif // NS_HEARTBEAT_HPP

