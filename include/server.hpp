/**
 * \project nswarm
 * \file server.hpp
 * \description A reliable TCP server that can be extended
 * for customized hooks.
 *
 * Copyright (c) 2019 Kevin Morris
 * All Rights Reserved.
 **/
#ifndef NS_SERVER_HPP
#define NS_SERVER_HPP

#include "async.hpp"
#include "logging.hpp"
#include "types.hpp"
#include <memory>
#include <thread>

namespace ns
{

class tcp_connection : public async_io_object<tcp_connection>
{
public:
    tcp_connection(io_service &io, ssl::context &ctx)
    {
        m_socket = std::make_unique<tcp_socket>(io, ctx);
    }

    ~tcp_connection() = default;

    tcp_connection(const tcp_connection &) = delete;
    void operator=(const tcp_connection &) = delete;

    tcp_connection(tcp_connection &&other)
    {
        *this = std::move(other);
    }

    void operator=(tcp_connection &&other)
    {
        m_socket = std::move(other.m_socket);
    }

    tcp_socket &socket()
    {
        return *m_socket;
    }

    void run()
    {
        // Start the ssl handshake which starts reading after
        m_socket->async_handshake(
            ssl::stream_base::server,
            boost::bind(&tcp_connection::async_on_handshake,
                        this->shared_from_this(),
                        boost::asio::placeholders::error));
    }

    // Make this publicly accessible
    using async_io_object::close;

}; // class tcp_connection

template <typename... Args>
std::shared_ptr<tcp_connection> make_tcp_connection(Args &&... args)
{
    return std::make_shared<tcp_connection>(std::forward<Args>(args)...);
}

class tcp_server : public async_object<tcp_server, tcp_connection>
{
public:
    tcp_server(unsigned short port)
        : m_io_ptr(std::make_unique<io_service>())
        , m_context(ssl::context::sslv23)
        , m_acceptor(*m_io_ptr, tcp::endpoint(tcp::v4(), port))
    {
    }

    tcp_server(io_service &io, unsigned short port)
        : m_context(ssl::context::sslv23)
        , m_acceptor(io, tcp::endpoint(tcp::v4(), port))
    {
    }

    ~tcp_server() = default;

    std::shared_ptr<tcp_server> use_certificate(const std::string &cert,
                                                const std::string &key)
    {
        m_context.use_certificate_file(cert, ssl::context::file_format::pem);
        m_context.use_private_key_file(key, ssl::context::file_format::pem);
        return this->shared_from_this();
    }

    // Since our standard on_error will be propagated to
    // connected clients, we need a custom error handler here
    // to allow callback actions on server errors.
    //
    // server->on_error([](auto c, const auto& e) {
    //         ilog("Connection had an error");
    //     })
    //     ->on_server_error([](auto srv, const auto& e) {
    //         ilog("Server had an error");
    //     });
    //
    std::shared_ptr<tcp_server>
    on_server_error(async_error_function<tcp_server> error_f)
    {
        m_server_error_f = error_f;
        return this->shared_from_this();
    }

    std::shared_ptr<tcp_server>
    on_accept(async_accept_function<tcp_connection> accept_f)
    {
        m_accept_f = accept_f;
        return this->shared_from_this();
    }

    void run()
    {
        m_acceptor.listen();
        try_accept();
        if (m_io_ptr)
            m_io_ptr->run();
    }

    // Start the server by calling run() within a thread
    void start()
    {
        logd("starting thread");
        m_thread = std::thread([this] { run(); });
        // Purposely sleep this function for 1/4th of a second
        // to allow the thread to start up and begin accepting
        // clients.
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    // Completely stop the server. Reset the associated
    // io_service and join the accept thread.
    void stop()
    {
        logd("stopping thread");
        m_io_ptr->stop();
        m_thread.join();
    }

protected:
    template <typename... Args>
    void call_accept(Args &&... args)
    {
        m_accept_f(std::forward<Args>(args)...);
    }

    bool has_accept() const
    {
        return m_accept_f != nullptr;
    }

    template <typename... Args>
    void call_server_error(Args &&... args)
    {
        m_server_error_f(std::forward<Args>(args)...);
    }

    bool has_server_error() const
    {
        return m_server_error_f != nullptr;
    }

private:
    void try_accept()
    {
        auto client =
            make_tcp_connection(m_acceptor.get_io_service(), m_context);
        m_acceptor.async_accept(client->socket().lowest_layer(),
                                boost::bind(&tcp_server::async_on_accept, this,
                                            boost::asio::placeholders::error,
                                            client));
    }

    void async_on_accept(const boost::system::error_code &ec,
                         std::shared_ptr<tcp_connection> client)
    {
        if (!ec) {
            logd("client accepted");

            if (this->has_connect())
                client->on_connect(
                    [this](auto client) { this->call_connect(client); });

            if (this->has_read())
                client->on_read([&](auto client, auto msg) {
                    this->call_read(client, msg);
                });

            if (this->has_close())
                client->on_close(
                    [this](auto client) { this->call_close(client); });

            if (this->has_error())
                client->on_error([this](auto client, const auto &ec) {
                    this->call_error(client, ec);
                });

            if (has_accept())
                call_accept(client); // client->run() should be called in the
                                     // accept function
            else
                logd("connection accepted, but no callback was provided");

            // Go for another accept loop and wait for a connection
            try_accept();
        } else {
            loge(ec.message());
            if (this->has_server_error())
                this->call_server_error(this->shared_from_this(), ec);
        }
    }

private:
    std::unique_ptr<io_service> m_io_ptr;

    async_accept_function<tcp_connection> m_accept_f;

    // A special callback just for server errors.
    async_error_function<tcp_server> m_server_error_f;

    std::thread m_thread;

protected:
    ssl::context m_context;
    tcp::acceptor m_acceptor;
}; // class tcp_server

template <typename... Args>
std::shared_ptr<tcp_server> make_tcp_server(Args &&... args)
{
    return std::make_shared<tcp_server>(std::forward<Args>(args)...);
}

}; // namespace ns

#endif
