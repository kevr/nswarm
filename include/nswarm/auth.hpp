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

#include <nswarm/data.hpp>
#include <nswarm/logging.hpp>
#include <atomic>
#include <string>

namespace ns
{

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

// layer 1 data
// T: auth_type
// A: action_type
template <typename T, typename A>
class auth : public data_object<auth<T, A>>
{
    std::string m_key;

    using auth_t = auth<T, A>;
    using data_object_t = data_object<auth_t>;

public:
    // Derive base constructors
    using data_object_t::data_object;

    // Data constructors.
    // With these two, we already have deduced our header values
    // and so we abstract away putting one together.
    auth(const std::string &msg)
        : data_object_t(serialize_header(data_type::auth::value, T::value,
                                         A::value, msg.size()),
                        msg)
    {
        logd("auth(string) constructor header: ",
             std::bitset<64>(this->header()), ", data: ", this->get_string());
    }

    auth(const json &js)
        : data_object_t(
              serialize_header(data_type::auth::value, T::value, A::value, 0),
              js)
    {
        set_size(this->get_string().size());

        logd("auth(json) constructor header: ", std::bitset<64>(this->header()),
             ", data: ", this->get_string());
    }

    // We need to provide auth level constructors for preparation
    // of internal member variables.
    auth(const auth &other)
        : data_object_t(other)
    {
        prepare();
    }

    auth &operator=(const auth &other)
    {
        data_object_t::operator=(other);
        return *this;
    }

    auth(auth &&other)
        : data_object_t(other)
    {
        prepare();
    }

    auth &operator=(auth &&other)
    {
        data_object_t::operator=(std::move(other));
        return *this;
    }

    const std::string &key() const
    {
        return m_key;
    }

    // We cannot call this in the base class constructor.
    // It is UB: the base constructor is run before the derived.
    //
    // We will provide an adhoc free function for creating
    // data_objects so that prepare will be done directly after constructed
    void prepare()
    {
        logd("setting up");
        parse_key();
        logd("found key: '", m_key, "'");
    }

private:
    const std::string &parse_key()
    {
        if (!m_key.size()) {
            // If we can't parse it, we just have an empty key.
            try {
                m_key = this->get_json().at("key");
            } catch (json::exception &e) {
                loge(e.what());
            }
        }
        return m_key;
    }
};

// Convenient aliases
template <typename T>
using auth_request = auth<T, action_type::request>;

template <typename T>
using auth_response = auth<T, action_type::response>;

/*
#ifdef ENABLE_SHA256
using auth_type = authentication::sha256;
#else
using auth_type = authentication::plain;
#endif
*/

}; // namespace ns

#endif // NS_AUTH_HPP
