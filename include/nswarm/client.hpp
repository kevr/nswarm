/**
 * \project nswarm
 * \file client.hpp
 * \description A reliable TCP client that can be reused for customizable
 * connections to servers.
 *
 * Copyright (c) 2019 Kevin Morris
 * All Rights Reserved.
 **/
#ifndef NS_CLIENT_HPP
#define NS_CLIENT_HPP

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/bind.hpp>
#include <memory>
#include <nswarm/async.hpp>
#include <nswarm/logging.hpp>
#include <nswarm/types.hpp>
#include <set>

namespace ns
{

template <typename T>
class client : public async_io_object<T>
{
public:
    // Create a client with an independent io_service
    client() noexcept
        : m_io_ptr(std::make_unique<io_service>())
        , m_context(ssl::context::sslv23)
        , m_resolver(*m_io_ptr)
    {
        this->initialize_socket(*m_io_ptr, m_context);

        // Override this outside as a user before connecting if you
        // wish to verify ssl certificates.
        set_verify_mode(ssl::context::verify_none);
        logd("default object created");
    }

    // Create a client with an external io_service
    client(io_service &io) noexcept
        : m_context(ssl::context::sslv23)
        , m_resolver(io)
    {
        this->initialize_socket(m_resolver.get_io_service(), m_context);

        // Override this outside as a user before connecting if you
        // wish to verify ssl certificates.
        this->set_verify_mode(ssl::context::verify_none);
        logd("object created with external io_service");
    }

    // We cannot copy clients around.
    client(const client &) = delete;
    void operator=(const client &) = delete;

    // Provide move construction and assignment
    client(client &&other) noexcept
        : m_io_ptr(std::move(other.m_io_ptr))
        , m_context(std::move(other.m_context))
        , m_resolver(std::move(other.m_resolver))
    {
        this->socket_ptr() = std::move(other.socket_ptr());
    }

    void operator=(client &&other) noexcept
    {
        m_io_ptr = std::move(other.m_io_ptr);
        m_context = std::move(other.m_context);
        this->socket_ptr() = std::move(other.socket_ptr());
        m_resolver = std::move(other.m_resolver);
    }

    ~client() noexcept = default;

    template <typename VerifyMode>
    void set_verify_mode(VerifyMode mode)
    {
        this->socket().set_verify_mode(mode);
    }

    // make this publicly accessible
    using async_io_object<T>::close;

    // run/stop wrap around an optional m_io_ptr
    void run(const std::string &host, const std::string &port) noexcept
    {
        connect(host, port);
        if (m_io_ptr)
            m_io_ptr->run();
    }

    void stop()
    {
        if (m_io_ptr)
            m_io_ptr->stop();
        else
            this->socket().get_io_service().stop();
    }

private:
    void connect(const std::string &host, const std::string &port) noexcept
    {
        this->initialize_socket(m_resolver.get_io_service(), m_context);
        this->start_resolve(m_resolver, host, port);
    }

private:
    std::unique_ptr<io_service> m_io_ptr;

    tcp::resolver m_resolver;

protected:
    ssl::context m_context;

    // Host/port we use to
    std::string m_host;
    std::string m_port;

    set_log_address;
};

class tcp_client : public client<tcp_client>
{
public:
    using client::client;
};

// Factory function for creating a client. Always use this
// function to create one, since we require that client
// is created set into a shared_ptr before running it's async
// functions.
template <typename... Args>
std::shared_ptr<tcp_client> make_tcp_client(Args &&... args)
{
    return std::make_shared<tcp_client>(std::forward<Args>(args)...);
}

}; // namespace ns

#endif
