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

heartbeat make_heartbeat_error(const std::string &error)
{
    // Create a normal impl
    auto hb = make_heartbeat<action::type::response>();

    // Then, set the error field in json...
    auto js = hb.get_json();
    js["error"] = error;
    auto size = js.dump().size(); // acquiire new size

    // And update the header with error::type::set and the new json
    hb.update(net::header(hb.get_type(), hb.head().args(), error::type::set,
                          hb.get_action(), size),
              js);

    return hb;
}

}; // namespace net
}; // namespace ns

#endif // NS_HEARTBEAT_HPP

