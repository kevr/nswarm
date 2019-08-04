/**
 * \prject nswarm
 * \file auth.hpp
 * \brief Authentication utilities and structures.
 *
 * Copyright (c) 2019 Kevin Morris
 * All Rights Reserved.
 **/
#ifndef NS_AUTH_HPP
#define NS_AUTH_HPP

#include <atomic>
#include <nswarm/data.hpp>
#include <nswarm/logging.hpp>
#include <string>

namespace ns
{

namespace net
{

class auth : public json_message
{
protected:
    std::string m_key;

public:
    using json_message::json_message;

    LAYER_CONSTRUCTOR(auth, key)

    const std::string &key() const
    {
        return m_key;
    }

private:
    template <action::type>
    friend net::auth make_auth(ns::json);
};

template <action::type action_t>
net::auth make_auth(ns::json js)
{
    const auto &str = js.dump();
    auto a = net::auth(net::header(message::type::auth, 0, error::type::none,
                                   action_t, str.size()),
                       js);
    a.m_key = a.get_json().at("key");
    return a;
}

template <typename... Args>
net::auth make_auth_request(Args &&... args)
{
    return make_auth<action::type::request>(std::forward<Args>(args)...);
}

template <typename... Args>
net::auth make_auth_response(Args &&... args)
{
    return make_auth<action::type::request>(std::forward<Args>(args)...);
}

inline net::auth make_auth_error(ns::json js)
{
    auto a = make_auth<action::type::response>(std::move(js));
    a.update(net::header(a.get_type(), a.head().args(), error::type::set,
                         a.get_action(), a.head().size()));
    return a;
}

inline net::auth make_auth_error(const std::string &error)
{
    ns::json js{{"error", error}};
    return make_auth_error(std::move(js));
}

inline net::auth make_auth_error(ns::json js, const std::string &error)
{
    js["error"] = error;
    return make_auth_error(std::move(js));
}

}; // namespace net

namespace value
{

// Types of authentications that can be done
enum auth_value { key = 0 };

}; // namespace value

namespace auth_type
{
struct key {
    static constexpr value::auth_value value = value::auth_value::key;
};

using variant = std::variant<key>;

inline variant deduce(const value::auth_value a)
{
    switch (a) {
    case auth_type::key::value:
        return key();
    }
    throw std::invalid_argument("auth_value unsupported: " + std::to_string(a));
}

}; // namespace auth_type

namespace authentication
{

struct plain {
};
struct sha256 {
};

template <typename T>
struct method {
    static bool compare(const std::string &src, const std::string &tgt);
};

template <>
struct method<plain> {
    static bool compare(const std::string &src, const std::string &tgt)
    {
        return src == tgt;
    }
};

template <>
struct method<sha256> {
    static bool compare(const std::string &src, const std::string &tgt)
    {
        return src == tgt;
    }
};
// M = method type: plain|sha256
template <typename M>
class context
{
    std::string m_key;
    std::atomic<bool> m_authed = false;

public:
    context() = default;

    // stored_key should be the key we'll compare
    // incoming authentications against.
    // If M = plain, this should be the plain key.
    // If M = sha256, this should be the sha256 hash of the key
    context(const std::string &stored_key)
        : m_key(stored_key)
    {
    }

    context(const context &ctx)
        : m_key(ctx.m_key)
        , m_authed(ctx.m_authed.load())
    {
    }

    void operator=(const context &ctx)
    {
        m_key = ctx.m_key;
        m_authed = ctx.m_authed.load();
    }

    context(context &&ctx)
        : m_key(std::move(ctx.m_key))
        , m_authed(ctx.m_authed.load())
    {
        ctx.m_authed.exchange(false);
    }

    void operator=(context &&ctx)
    {
        m_key = std::move(ctx.m_key);
        m_authed.exchange(ctx.m_authed.load());
        ctx.m_authed.exchange(false);
    }

    ~context() = default;

    bool compare(const std::string &key)
    {
        logd("comparing '", key, "' against stored '", m_key, "'");
        return method<M>::compare(key, m_key);
    }

    bool authenticate(const std::string &key)
    {
        if (m_key.size() == 0) {
            loge("cannot auth against an empty target key");
            return false;
        }
        m_authed.exchange(compare(key));
        return m_authed.load();
    }

    const bool authenticated() const
    {
        return m_authed.load();
    }

    const std::string &key() const
    {
        return m_key;
    }
};

}; // namespace authentication

template <typename T>
using auth_context = authentication::context<T>;

/*
#ifdef ENABLE_SHA256
using auth_type = authentication::sha256;
#else
using auth_type = authentication::plain;
#endif
*/

}; // namespace ns

#endif // NS_AUTH_HPP
