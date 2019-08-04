#ifndef NS_PROTOCOL_HPP
#define NS_PROTOCOL_HPP

#include <boost/asio.hpp>
#include <functional>
#include <map>
#include <memory>
#include <nswarm/data.hpp>

namespace ns
{

template <typename T, typename D>
using async_protocol_function = std::function<void(std::shared_ptr<T>, D)>;

template <typename T, typename D>
using async_auth_function = async_protocol_function<T, D>;

template <typename T, typename D>
using async_implement_function = async_protocol_function<T, D>;

template <typename T, typename D>
using async_subscribe_function = async_protocol_function<T, D>;

template <typename T, typename D>
using async_task_function = async_protocol_function<T, D>;

template <typename ConnectionT, typename DataT>
class protocol
{
private:
    using async_protocol_function =
        ns::async_protocol_function<ConnectionT, DataT>;

public:
    void set_external_log_address(const ConnectionT *conn)
    {
        p_secret_log_addr = std::to_string((unsigned long)conn);
    }

    protocol &on_auth(async_auth_function<ConnectionT, DataT> auth_f)
    {
        m_auth_f = auth_f;
        call_table[net::message::auth] = m_auth_f;
        return *this;
    }

    template <typename... Args>
    void call_auth(Args &&... args)
    {
        return m_auth_f(std::forward<Args>(args)...);
    }

    bool has_auth() const
    {
        return m_auth_f != nullptr;
    }

    protocol &
    on_implement(async_implement_function<ConnectionT, DataT> implement_f)
    {
        m_implement_f = implement_f;
        call_table[net::message::implement] = m_implement_f;
        return *this;
    }

    template <typename... Args>
    void call_implement(Args &&... args)
    {
        return m_implement_f(std::forward<Args>(args)...);
    }

    bool has_implement() const
    {
        return m_implement_f != nullptr;
    }

    protocol &on_subscribe(async_protocol_function subscribe_f)
    {
        m_subscribe_f = subscribe_f;
        call_table[net::message::subscribe] = m_subscribe_f;
        return *this;
    }

    template <typename... Args>
    void call_subscribe(Args &&... args)
    {
        return m_subscribe_f(std::forward<Args>(args)...);
    }

    bool has_subscribe() const
    {
        return m_subscribe_f != nullptr;
    }

    protocol &on_task(async_protocol_function task_f)
    {
        m_task_f = task_f;
        call_table[net::message::task] = m_task_f;
        return *this;
    }

    template <typename... Args>
    void call_task(Args &&... args)
    {
        return m_task_f(std::forward<Args>(args)...);
    }

    bool has_task() const
    {
        return m_task_f != nullptr;
    }

    template <typename... Args>
    void call(net::message::type type, Args &&... args)
    {
        call_table.at(type)(std::forward<Args>(args)...);
    }

private:
    async_protocol_function m_auth_f;
    async_protocol_function m_implement_f;
    async_protocol_function m_subscribe_f;
    async_protocol_function m_task_f;

    std::map<net::message::type, async_protocol_function> call_table;

    set_log_address;
};

namespace error
{
}; // namespace error

}; // namespace ns

#endif
