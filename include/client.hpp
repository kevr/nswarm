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

#include "async.hpp"
#include "logging.hpp"
#include "types.hpp"
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/bind.hpp>
#include <memory>
#include <set>

namespace ns
{

class tcp_client : public async_io_object<tcp_client>
{
public:
    // Create a tcp_client with an independent io_service
    tcp_client()
        : m_io_ptr(std::make_unique<io_service>())
        , m_context(ssl::context::sslv23)
        , m_resolver(*m_io_ptr)
    {
        m_socket = std::make_unique<socket>(*m_io_ptr, m_context);
        // Override this outside as a user before connecting if you
        // wish to verify ssl certificates.
        set_verify_mode(ssl::context::verify_none);
        logd("Default object created");
    }

    // Create a tcp_client with an external io_service
    tcp_client(io_service &io)
        : m_context(ssl::context::sslv23)
        , m_resolver(io)
    {
        m_socket = std::make_unique<socket>(io, m_context);
        // Override this outside as a user before connecting if you
        // wish to verify ssl certificates.
        set_verify_mode(ssl::context::verify_none);
        logd("Object created with external io_service");
    }

    // We cannot copy clients around.
    tcp_client(const tcp_client &) = delete;
    void operator=(const tcp_client &) = delete;

    // Provide move construction and assignment
    tcp_client(tcp_client &&other)
        : m_io_ptr(std::move(other.m_io_ptr))
        , m_context(std::move(other.m_context))
        , m_resolver(std::move(other.m_resolver))
    {
        m_socket = std::move(other.m_socket);
    }

    void operator=(tcp_client &&other)
    {
        m_io_ptr = std::move(other.m_io_ptr);
        m_context = std::move(other.m_context);
        m_socket = std::move(other.m_socket);
        m_resolver = std::move(other.m_resolver);
    }

    ~tcp_client() = default;

    const bool connected() const
    {
        return m_socket->lowest_layer().is_open();
    }

    template <typename VerifyMode>
    void set_verify_mode(VerifyMode mode)
    {
        m_socket->set_verify_mode(mode);
    }

    void close()
    {
        if (connected()) {
            async_io_object<tcp_client>::close(); // ssl stream shutdown
        }
    }

    // run/stop wrap around an optional m_io_ptr
    void run(const std::string &host, const std::string &port)
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
            m_socket->get_io_service().stop();
    }

private:
    void connect(const std::string &host, const std::string &port)
    {
        m_host = host;
        m_port = port;

        tcp::resolver::query query(m_host, m_port);
        m_resolver.async_resolve(
            query,
            boost::bind(&tcp_client::async_on_resolve, shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::iterator));
    }

    void store_endpoint(const tcp::resolver::endpoint_type ep)
    {
        m_remote_host = "";
        m_remote_port = "";
    }

private:
    std::unique_ptr<io_service> m_io_ptr;

    std::string m_host;
    std::string m_port;

    tcp::resolver m_resolver;

    std::string m_remote_host;
    std::string m_remote_port;

protected:
    ssl::context m_context;

    // Error whitelist - when encountering one of these errors,
    // we will gracefully close the socket.
    static inline const std::set<boost::system::error_code> m_errors_wl{
        boost::asio::ssl::error::stream_truncated,
        boost::asio::error::operation_aborted,
        boost::asio::error::connection_aborted,
        boost::asio::error::connection_reset,
        boost::asio::error::connection_refused};
};

// Factory function for creating a tcp_client. Always use this
// function to create one, since we require that tcp_client
// is created set into a shared_ptr before running it's async
// functions.
template <typename... Args>
std::shared_ptr<tcp_client> make_tcp_client(Args &&... args)
{
    return std::make_shared<tcp_client>(std::forward<Args>(args)...);
}

}; // namespace ns

#endif
