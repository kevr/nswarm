#ifndef NS_PROTOCOL_HPP
#define NS_PROTOCOL_HPP

#include <boost/asio.hpp>
#include <functional>
#include <map>
#include <memory>
#include <nswarm/auth.hpp>
#include <nswarm/data.hpp>
#include <nswarm/heartbeat.hpp>
#include <nswarm/implement.hpp>
#include <nswarm/subscribe.hpp>
#include <nswarm/task.hpp>

namespace ns
{

template <typename T, typename D>
using async_protocol_function = std::function<void(std::shared_ptr<T>, D)>;

template <typename T>
using async_auth_function = async_protocol_function<T, net::auth>;

template <typename T>
using async_implement_function =
    async_protocol_function<T, net::implementation>;

template <typename T>
using async_subscribe_function = async_protocol_function<T, net::subscription>;

template <typename T>
using async_task_function = async_protocol_function<T, net::task>;

template <typename T>
using async_heartbeat_function = async_protocol_function<T, net::heartbeat>;

template <typename ConnectionT, typename DataT>
class protocol
{
private:
    template <typename D>
    using async_protocol_function = ns::async_protocol_function<ConnectionT, D>;

    async_protocol_function<net::auth> m_auth_f{[this](auto c, auto d) {
        logd("default auth_f function called");
    }};
    async_protocol_function<net::implementation> m_implement_f{
        [this](auto c, auto d) {
            logd("default implement_f function called");
        }};
    async_protocol_function<net::subscription> m_subscribe_f{
        [this](auto c, auto d) {
            logd("default subscribe_f function called");
        }};
    async_protocol_function<net::task> m_task_f{[this](auto c, auto d) {
        logd("default task_f function called");
    }};
    async_protocol_function<net::heartbeat> m_heartbeat_f{
        [this](auto c, auto d) {
            logd("default heartbeat_f function called");
            d.update_action(net::action::type::response);
            c->send(std::move(d));
        }};

protected:
    set_log_address;

public:
    protocol()
    {
        on_heartbeat([](auto c, auto m) -> void {
        });
    }

    void set_external_log_address(const ConnectionT *conn)
    {
        p_secret_log_addr = std::to_string((unsigned long)conn);
    }

    protocol &on_auth(async_protocol_function<net::auth> auth_f)
    {
        m_auth_f = auth_f;
        return *this;
    }

    template <typename... Args>
    void call_auth(Args &&... args)
    {
        m_auth_f(std::forward<Args>(args)...);
    }

    bool has_auth() const
    {
        return m_auth_f != nullptr;
    }

    protocol &
    on_implement(async_protocol_function<net::implementation> implement_f)
    {
        m_implement_f = implement_f;
        return *this;
    }

    template <typename... Args>
    void call_implement(Args &&... args)
    {
        m_implement_f(std::forward<Args>(args)...);
    }

    bool has_implement() const
    {
        return m_implement_f != nullptr;
    }

    protocol &
    on_subscribe(async_protocol_function<net::subscription> subscribe_f)
    {
        m_subscribe_f = subscribe_f;
        return *this;
    }

    template <typename... Args>
    void call_subscribe(Args &&... args)
    {
        m_subscribe_f(std::forward<Args>(args)...);
    }

    bool has_subscribe() const
    {
        return m_subscribe_f != nullptr;
    }

    protocol &on_task(async_protocol_function<net::task> task_f)
    {
        m_task_f = task_f;
        return *this;
    }

    template <typename... Args>
    void call_task(Args &&... args)
    {
        m_task_f(std::forward<Args>(args)...);
    }

    bool has_task() const
    {
        return m_task_f != nullptr;
    }

    protocol &on_heartbeat(async_protocol_function<net::heartbeat> heartbeat_f)
    {
        m_heartbeat_f = heartbeat_f;
        return *this;
    }

    template <typename... Args>
    void call_heartbeat(Args &&... args)
    {
        m_heartbeat_f(std::forward<Args>(args)...);
    }

    bool has_heartbeat() const
    {
        return m_heartbeat_f != nullptr;
    }

    template <typename... Args>
    void call(net::message::type type, Args &&... args)
    {
        match(
            net::message::deduce(type),
            [&](net::message::tag::auth) {
                call_auth(std::forward<Args>(args)...);
            },
            [&](net::message::tag::implement) {
                call_implement(std::forward<Args>(args)...);
            },
            [&](net::message::tag::subscribe) {
                call_subscribe(std::forward<Args>(args)...);
            },
            [&](net::message::tag::task) {
                call_task(std::forward<Args>(args)...);
            },
            [&](net::message::tag::heartbeat) {
                call_heartbeat(std::forward<Args>(args)...);
            },
            [&](auto e) {
                loge("invalid message type received: ", (uint16_t)type,
                     ", bailing");
            });
    }
};

namespace error
{
}; // namespace error

}; // namespace ns

#endif
