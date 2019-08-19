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

#include <functional>
#include <memory>
#include <nswarm/async.hpp>
#include <nswarm/logging.hpp>
#include <nswarm/types.hpp>
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
    boost::asio::deadline_timer m_heartbeat;
    boost::asio::streambuf m_heartbeat_buf;
    std::ostream m_heartbeat_os{&m_heartbeat_buf};

protected:
    set_log_address;

public:
    connection(io_service &io, ssl::context &ctx) noexcept
        : m_heartbeat(io)
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

    void start_heartbeat()
    {
        auto hb = net::make_heartbeat_request();
        uint64_t head = hb.head().value();
        m_heartbeat_os.write((char *)&head, sizeof(uint64_t));
        const auto &s = hb.get_string();
        if (s.size())
            m_heartbeat_os.write(s.data(), s.size());
        boost::asio::async_write(
            *this->socket_ptr(), m_heartbeat_buf,
            boost::asio::transfer_exactly(s.size() + sizeof(uint64_t)),
            boost::bind(&connection<T>::async_on_heartbeat_write,
                        this->shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
    }

    // Make this publicly accessible
    using async_io_object<T>::close;

private:
    void async_on_heartbeat_write(const boost::system::error_code &ec,
                                  std::size_t bytes)
    {
        if (ec) {
            loge("got error while sending heartbeat: ", ec.message());
            if (this->connected()) {
                this->close();
            }
        } else {
            // We send a heartbeat every delay seconds
            const boost::posix_time::seconds delay{30}; // Seconds
            m_heartbeat.expires_from_now(delay);
            m_heartbeat.async_wait(boost::bind(&connection<T>::start_heartbeat,
                                               this->shared_from_this()));
        }
    }

protected:
    template <typename U>
    friend class tcp_server;

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
    std::unique_ptr<io_service> m_io_ptr;

    // Set defaults for all of these handlers. We don't require them
    // externally to operate.
    async_accept_function<T> m_accept_f{[this](auto c) -> void {
        logd("default m_accept_f called");
    }};

    async_server_error_function<tcp_server> m_server_error_f{
        [this](auto c, auto e) -> void {
            logd("default m_server_error_f called");
        }};

    // Set a function by default here
    async_close_function<T> m_removed_f{[this](auto c) -> void {
        logd("default m_removed_f called");
    }};

    // Start connection count at 0. on_close/on_error we will
    // decrement this count.
    std::atomic<std::size_t> m_connections = 0;

    // Server execution state.
    std::thread m_thread;
    std::mutex m_mutex;
    bool m_running = false;

protected:
    ssl::context m_context;
    tcp::acceptor m_acceptor;

protected:
    set_log_address;

public:
    tcp_server(const std::string &host, unsigned short port)
        : m_io_ptr(std::make_unique<io_service>())
        , m_context(ssl::context::sslv23)
        , m_acceptor(
              *m_io_ptr,
              tcp::endpoint(boost::asio::ip::address::from_string(host), port))
    {
    }

    tcp_server(io_service &io, const std::string &host, unsigned short port)
        : m_context(ssl::context::sslv23)
        , m_acceptor(io, tcp::endpoint(
                             boost::asio::ip::address::from_string(host), port))
    {
    }

    tcp_server(unsigned short port) noexcept
        : m_io_ptr(std::make_unique<io_service>())
        , m_context(ssl::context::sslv23)
        , m_acceptor(*m_io_ptr, tcp::endpoint(tcp::v4(), port))
    {
        logd("using port ", port);
    }

    tcp_server(io_service &io, unsigned short port) noexcept
        : m_context(ssl::context::sslv23)
        , m_acceptor(io, tcp::endpoint(tcp::v4(), port))
    {
        logd("using port ", port);
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

    tcp_server &on_removed(async_close_function<T> removed_f)
    {
        m_removed_f = removed_f;
        return *this;
    }

    void run()
    {
        m_acceptor.listen();
        logi("accepting new connections on ", host(), ":", port());
        try_accept();
        if (m_io_ptr)
            m_io_ptr->run();
    }

    const std::string host() const
    {
        return m_acceptor.local_endpoint().address().to_v4().to_string();
    }

    const std::string port() const
    {
        return std::to_string(m_acceptor.local_endpoint().port());
    }

    // Start the server by calling run() within a thread
    void start()
    {
        std::lock_guard<std::mutex> g(m_mutex);
        if (m_running)
            return;

        if (!has_accept())
            loge("start() called on a server with no on_accept provided");

        logd("starting thread");
        m_thread = std::thread([this] {
            run();
        });
        // Purposely sleep this function for 1/4th of a second
        // to allow the thread to start up and begin accepting
        // clients.
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        m_running = true;
    }

    // Completely stop the server. Reset the associated
    // io_service and join the accept thread.
    void stop()
    {
        std::lock_guard<std::mutex> g(m_mutex);
        if (!m_running)
            return;

        logd("stopping thread");
        if (m_io_ptr) {
            m_io_ptr->stop();
            logd("reset io_service");
        }
        m_thread.join();
        m_running = false;
    }

    io_service &get_io_service()
    {
        if (m_io_ptr)
            return *m_io_ptr;
        return m_acceptor.get_io_service();
    }

    const std::size_t count() const
    {
        return m_connections.load();
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

    template <typename... Args>
    void call_removed(Args &&... args) noexcept
    {
        m_removed_f(std::forward<Args>(args)...);
    }

    bool has_removed() const noexcept
    {
        return m_removed_f != nullptr;
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

            m_connections.exchange(m_connections.load() + 1);

            // Bind all callbacks to our new connection
            if (this->has_connect())
                client->on_connect([this](auto c) {
                    c->start_heartbeat();
                    this->call_connect(c);
                });
            if (this->has_read())
                client->on_read(std::bind(
                    &tcp_server::template call_read<std::shared_ptr<T>,
                                                    json_message>,
                    this, std::placeholders::_1, std::placeholders::_2));
            if (this->has_close())
                client->on_close([this](auto c) {
                    m_connections.exchange(m_connections.load() - 1);
                    this->call_close(c);
                    call_removed(c);
                });
            if (this->has_error())
                client->on_error([this](auto c, const auto &ec) {
                    m_connections.exchange(m_connections.load() - 1);
                    this->call_error(c, ec);
                    call_removed(c);
                });

            // If we're provided with an accept callback, run that
            if (has_accept())
                call_accept(client); // client->run() should be called
                                     // in the accept function
            m_running = true;
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
