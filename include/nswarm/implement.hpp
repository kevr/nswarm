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

// Inline to avoid ODR violation
inline implementation make_impl_error(const std::string &method,
                                      const std::string &error)
{
    auto impl = net::make_implementation<action::type::response>(method);
    impl.update_error(error::type::set, error);
    return impl;
}

}; // namespace net
}; // namespace ns

#endif // NS_IMPLEMENT_HPP

