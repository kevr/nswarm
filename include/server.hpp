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
#include <functional>
#include <memory>
#include <thread>

namespace ns
{

template <typename T>
using async_accept_function = std::function<void(std::shared_ptr<T>)>;

template <typename T>
using async_server_error_function =
    std::function<void(T *, const boost::system::error_code &)>;

template <typename T>
class connection : public async_io_object<T>
{
public:
    connection(io_service &io, ssl::context &ctx) noexcept
    {
        this->initialize_socket(io, ctx);
    }

    ~connection() noexcept = default;

    connection(const connection &) = delete;
    void operator=(const connection &) = delete;

    connection(connection &&other) noexcept
    {
        *this = std::move(other);
    }

    void operator=(connection &&other) noexcept
    {
        this->socket() = std::move(other.m_socket);
    }

    void run() noexcept
    {
        this->start_handshake(ssl::stream_base::server);
    }

    // Make this publicly accessible
    using async_io_object<T>::close;

protected:
    set_log_address;
}; // class connection

template <typename T, typename... Args>
std::shared_ptr<T> make_connection(Args &&... args)
{
    return std::make_shared<connection<T>>(std::forward<Args>(args)...);
}

// T = connection type
template <typename T>
class tcp_server : public async_object<tcp_server<T>, T>
{
public:
    tcp_server(unsigned short port) noexcept
        : m_io_ptr(std::make_unique<io_service>())
        , m_context(ssl::context::sslv23)
        , m_acceptor(*m_io_ptr, tcp::endpoint(tcp::v4(), port))
    {
    }

    tcp_server(io_service &io, unsigned short port) noexcept
        : m_context(ssl::context::sslv23)
        , m_acceptor(io, tcp::endpoint(tcp::v4(), port))
    {
    }

    virtual ~tcp_server() = default;

    tcp_server &use_certificate(const std::string &cert,
                                const std::string &key) noexcept
    {
        m_context.use_certificate_file(cert, ssl::context::file_format::pem);
        m_context.use_private_key_file(key, ssl::context::file_format::pem);
        return *this;
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
    tcp_server &on_server_error(async_server_error_function<tcp_server> error_f)
    {
        m_server_error_f = error_f;
        return *this;
    }

    tcp_server &on_accept(async_accept_function<T> accept_f)
    {
        m_accept_f = accept_f;
        return *this;
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
        if (!has_accept())
            loge("start() called on a server with no on_accept provided");

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
        if (m_io_ptr) {
            m_io_ptr->stop();
            logd("reset io_service");
        }
        m_thread.join();
    }

    io_service &get_io_service()
    {
        return *m_io_ptr;
    }

    const std::size_t count() const
    {
        return m_connections;
    }

protected:
    template <typename... Args>
    void call_accept(Args &&... args) noexcept
    {
        m_accept_f(std::forward<Args>(args)...);
    }

    bool has_accept() const noexcept
    {
        return m_accept_f != nullptr;
    }

    template <typename... Args>
    void call_server_error(Args &&... args) noexcept
    {
        m_server_error_f(std::forward<Args>(args)...);
    }

    bool has_server_error() const noexcept
    {
        return m_server_error_f != nullptr;
    }

private:
    void try_accept()
    {
        auto client =
            std::make_shared<T>(m_acceptor.get_io_service(), m_context);
        m_acceptor.async_accept(client->socket().lowest_layer(),
                                boost::bind(&tcp_server::async_on_accept, this,
                                            boost::asio::placeholders::error,
                                            client));
    }

    void async_on_accept(const boost::system::error_code &ec,
                         std::shared_ptr<T> client) noexcept
    {
        if (!ec) {
            logd("client accepted");

            ++m_connections; // increment connection count

            if (this->has_connect())
                client->on_connect(std::bind(
                    &tcp_server::template call_connect<std::shared_ptr<T>>,
                    this, std::placeholders::_1));

            if (this->has_read())
                client->on_read(std::bind(
                    &tcp_server::template call_read<std::shared_ptr<T>, data>,
                    this, std::placeholders::_1, std::placeholders::_2));

            if (this->has_close())
                client->on_close([this](auto c) {
                    --m_connections; // remove connection from count
                    this->call_close(c);
                });

            if (this->has_error())
                client->on_error([this](auto c, const auto &ec) {
                    --m_connections; // remove connection from count
                    this->call_error(c, ec);
                });

            if (has_accept())
                call_accept(client); // client->run() should be called
                                     // in the accept function
            else
                logd("connection accepted, but no callback was "
                     "provided");

            // Go for another accept loop and wait for a connection
            try_accept();
        } else {
            loge(ec.message());
            if (this->has_server_error())
                this->call_server_error(this, ec);
        }
    }

private:
    std::unique_ptr<io_service> m_io_ptr;

    async_accept_function<T> m_accept_f;
    async_server_error_function<tcp_server> m_server_error_f;

    std::thread m_thread;

    // Start connection count at 0. on_close/on_error we will
    // decrement this count.
    std::size_t m_connections = 0;

protected:
    ssl::context m_context;
    tcp::acceptor m_acceptor;

    set_log_address;
}; // class tcp_server

// raw tcp_connection. other classes should derive from connection<DerivedT>
class tcp_connection : public connection<tcp_connection>
{
public:
    using connection::connection;
};

// T = connection type
template <typename T = tcp_connection, typename... Args>
std::shared_ptr<tcp_server<T>> make_tcp_server(Args &&... args)
{
    return std::make_shared<tcp_server<T>>(std::forward<Args>(args)...);
}

}; // namespace ns

#endif
