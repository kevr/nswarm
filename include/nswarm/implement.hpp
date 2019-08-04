#ifndef NS_IMPLEMENT_HPP
#define NS_IMPLEMENT_HPP

#include <nswarm/data.hpp>

namespace ns
{

namespace net
{

class implementation : public json_message
{
    std::string m_method;

public:
    using json_message::json_message;

    // Include overloads that sync m_method
    LAYER_CONSTRUCTOR(implementation, method)

    const std::string &method() const
    {
        return m_method;
    }

private:
    template <action::type>
    friend implementation make_implementation(const std::string &);
};

template <action::type action_t>
implementation make_implementation(const std::string &method)
{
    ns::json js{{"method", method}};
    auto impl = implementation(net::header(message::type::implement, 0,
                                           error::type::none, action_t,
                                           js.dump().size()),
                               js);
    impl.m_method = impl.get_json().at("method");
    return impl;
}

template <typename... Args>
implementation make_impl_request(Args &&... args)
{
    return make_implementation<action::type::request>(
        std::forward<Args>(args)...);
}

template <typename... Args>
implementation make_impl_response(Args &&... args)
{
    return make_implementation<action::type::response>(
        std::forward<Args>(args)...);
}

implementation make_impl_error(const std::string &method,
                               const std::string &error)
{
    // Create a normal impl
    auto impl = make_implementation<action::type::response>(method);

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

#endif // NS_IMPLEMENT_HPP

