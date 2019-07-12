#ifndef NS_VARIANT_HPP
#define NS_VARIANT_HPP

#include <type_traits>
#include <variant>

namespace ns
{

// https://en.cppreference.com/w/cpp/utility/variant/visit
template <typename... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

template <typename... Ts>
overloaded(Ts...)->overloaded<Ts...>;

template <typename... Args, typename... Ts>
auto match(const std::variant<Args...> &var, Ts &&... ts)
{
    return std::visit(ns::overloaded{ts...}, var);
}

}; // namespace ns

#endif
