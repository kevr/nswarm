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

    implementation(const std::string &method_)
        : m_method(method_)
    {
        update(ns::json{{"method", method_}});
        update(net::header(message::type::implement, 0, error::type::none,
                           action::type::request, m_data.size()));
    }

    // Include overloads that sync m_method
    LAYER_CONSTRUCTOR(implementation, method)

    const std::string &method() const
    {
        return m_method;
    }

private:
    template <action::type, typename... Args>
    friend implementation make_implementation(Args &&...);
};

template <action::type action_t, typename... Args>
implementation make_implementation(Args &&... args)
{
    auto impl = implementation(std::forward<Args>(args)...);
    if (action_t != impl.get_action())
        impl.update_action(action_t);
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

