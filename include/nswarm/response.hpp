#ifndef NS_RESPONSE_HPP
#define NS_RESPONSE_HPP

#include <functional>

namespace ns
{

// typename T: Data type
template <typename T>
using async_response_function = std::function<void(T)>;

// typename T: Derivative class type
// typename U: Data type; default T
//
// class A : protected responsive<A, ns::json_message>;
// class B : protected responsive<B, ns::task>;
//
template <typename T, typename U = T>
class responsive
{
protected:
    async_response_function<U> m_response_f;

protected:
    void on_response(async_response_function<U> f)
    {
        m_response_f = f;
    }

    void call_response(U u)
    {
        m_response_f(std::move(u));
    }
};

}; // namespace ns

#endif

