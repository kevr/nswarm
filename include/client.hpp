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

namespace ns {

class tcp_client : public async_object<tcp_client>
{
  public:
    // Create a tcp_client with an independent io_service
    tcp_client()
        : m_io_ptr(std::make_unique<io_service>())
        , m_context(ssl::context::sslv23)
        , m_socket(std::make_unique<socket>(*m_io_ptr, m_context))
        , m_resolver(*m_io_ptr)
    {
        // Override this outside as a user before connecting if you
        // wish to verify ssl certificates.
        set_verify_mode(ssl::context::verify_none);
        logd("Default object created");
    }

    // Create a tcp_client with an external io_service
    tcp_client(io_service &io)
        : m_context(ssl::context::sslv23)
        , m_socket(std::make_unique<socket>(io, m_context))
        , m_resolver(io)
    {
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
        , m_socket(std::move(other.m_socket))
        , m_resolver(std::move(other.m_resolver))
    {}

    void operator=(tcp_client &&other)
    {
        m_io_ptr = std::move(other.m_io_ptr);
        m_context = std::move(other.m_context);
        m_socket = std::move(other.m_socket);
        m_resolver = std::move(other.m_resolver);
    }

    ~tcp_client() = default;

    void send(uint16_t type, uint16_t flags = 0, uint32_t data_size = 0,
              std::optional<std::string> data = std::optional<std::string>())
    {
        uint64_t pkt = serialize_packet(type, flags, data_size);
        m_os << pkt;
        if (data)
            m_os << *data;

        boost::asio::async_write(
            *m_socket, m_output,
            boost::asio::transfer_exactly(data_size + sizeof(uint64_t)),
            boost::bind(&tcp_client::async_on_write, this,
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
    }

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
            boost::system::error_code ec; // No need to check, silently fail
            m_socket->shutdown(ec);       // ssl stream shutdown
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
            query, boost::bind(&tcp_client::on_resolve, this,
                               boost::asio::placeholders::error,
                               boost::asio::placeholders::iterator));
    }

    // boost::asio async callbacks
    void on_resolve(const boost::system::error_code &ec,
                    tcp::resolver::iterator iter)
    {
        if (!ec) {
            logd("resolve succeeded");
            auto ep = iter++;
            boost::asio::async_connect(
                m_socket->lowest_layer(), ep,
                boost::bind(&tcp_client::async_on_connect, this,
                            boost::asio::placeholders::error, iter));
            // connect!
        } else {
            handle_error(ec, "unable to resolve address: ", ec.message());
        }
    }

    void async_on_connect(const boost::system::error_code &ec,
                          tcp::resolver::iterator iter)
    {
        if (!ec) {
            logd("connect succeeded");
            m_socket->async_handshake(
                boost::asio::ssl::stream_base::client,
                boost::bind(&tcp_client::async_on_handshake, this,
                            boost::asio::placeholders::error));
            // handshake
        } else if (iter != tcp::resolver::iterator()) {
            logd("connect failed, trying next address");
            auto ep = iter++;
            boost::asio::async_connect(
                m_socket->lowest_layer(), ep,
                boost::bind(&tcp_client::async_on_connect, this,
                            boost::asio::placeholders::error, iter));
        } else {
            handle_error(ec, "unable to connect: ", ec.message());
        }
    }

    void async_on_handshake(const boost::system::error_code &ec)
    {
        if (!ec) {
            logd("handshake succeeded");

            if (m_connect_f)
                m_connect_f(*this);

            boost::asio::async_read(
                *m_socket, m_input,
                boost::asio::transfer_exactly(sizeof(uint64_t)),
                boost::bind(&tcp_client::async_on_read_packet, this,
                            boost::asio::placeholders::error));
        } else {
            handle_error(ec, "client socket closed while handshaking");
        }
    }

    void async_on_read_packet(const boost::system::error_code &ec)
    {
        if (!ec) {
            uint64_t pkt;
            m_is >> pkt;

            auto [type, flags, size] = deserialize_packet(pkt);
            logd("received packet, type = ", type, ", flags = ", flags,
                 " size = ", size);

            if (size > 0) {
                boost::asio::async_read(
                    *m_socket, m_input, boost::asio::transfer_exactly(size),
                    boost::bind(&tcp_client::async_on_read_data, this,
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred,
                                pkt));
            } else {
                if (m_read_f) {
                    message msg(pkt, std::nullopt);
                    m_read_f(*this, msg);
                }
                boost::asio::async_read(
                    *m_socket, m_input,
                    boost::asio::transfer_exactly(sizeof(uint64_t)),
                    boost::bind(&tcp_client::async_on_read_packet, this,
                                boost::asio::placeholders::error));
            }
        } else {
            handle_error(ec, "client socket closed while reading data packet");
        }
    }

    void async_on_read_data(const boost::system::error_code &ec,
                            std::size_t bytes, uint64_t pkt)
    {
        if (!ec) {
            std::string data(bytes, '0');
            m_is.read(&data[0], bytes);
            logd("received ", bytes, " bytes of data");

            // Construct message, pass to read fn if provided
            if (m_read_f) {
                message msg(pkt, data);
                m_read_f(*this, msg);
            }

            boost::asio::async_read(
                *m_socket, m_input,
                boost::asio::transfer_exactly(sizeof(uint64_t)),
                boost::bind(&tcp_client::async_on_read_packet, this,
                            boost::asio::placeholders::error));
        } else {
            handle_error(ec, "client socket closed while reading data chunk");
        }
    }

    void async_on_write(const boost::system::error_code &ec, std::size_t bytes)
    {
        if (!ec) {
            logd("sent ", bytes, " bytes of data (", bytes - sizeof(uint64_t),
                 " data size)");
        } else {
            handle_error(ec, "client socket closed while writing");
        }
    }

    template <typename... Args>
    void handle_error(const boost::system::error_code &ec, Args &&... args)
    {
        // If the error is not in the whitelist, explicitly log it out
        if (m_errors_wl.find(ec) == m_errors_wl.end()) {
            loge(ec.message());
            close();
            if (m_error_f)
                m_error_f(*this, ec);
        } else {
            logd(std::forward<Args>(args)...);
            close();
            if (m_close_f)
                m_close_f(*this);
        }
    }

    void store_endpoint(const tcp::resolver::endpoint_type ep)
    {
        m_remote_host = "";
        m_remote_port = "";
    }

  private:
    boost::asio::streambuf m_input;
    std::istream m_is{&m_input};

    boost::asio::streambuf m_output;
    std::ostream m_os{&m_output};

    std::unique_ptr<io_service> m_io_ptr;

    std::string m_host;
    std::string m_port;

    tcp::resolver m_resolver;

    std::string m_remote_host;
    std::string m_remote_port;

  protected:
    ssl::context m_context;
    std::unique_ptr<socket> m_socket;

    // Error whitelist - when encountering one of these errors,
    // we will gracefully close the socket.
    static inline const std::set<boost::system::error_code> m_errors_wl{
        boost::asio::ssl::error::stream_truncated};
};

}; // namespace ns

#endif
