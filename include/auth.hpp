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

#include "logging.hpp"
#include <atomic>
#include <string>

namespace ns
{
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
        // return hash(src) == tgt;
        return false;
    }
};

// M = method type: plain|sha256
template <typename M>
class context
{
public:
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
        return method<M>::compare(key, m_key);
    }

    bool authenticate(const std::string &key)
    {
        m_authed.exchange(compare(key));
        return m_authed.load();
    }

    const bool authenticated() const
    {
        return m_authed.load();
    }

private:
    std::string m_key;
    std::atomic<bool> m_authed = false;
};

}; // namespace authentication

template <typename T>
using auth_context = authentication::context<T>;

}; // namespace ns

#endif // NS_AUTH_HPP
