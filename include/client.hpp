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
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/bind.hpp>
#include <memory>

namespace ns {

// This class *must* be created as a shared_ptr
class tcp_client : public std::enable_shared_from_this<tcp_client>,
                   public async_object<tcp_client>
{
  private: // Some upfront aliases
    using sock_t = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;

  public:
    // Create a tcp_client with an independent io_service
    tcp_client()
        : m_io_ptr(std::make_unique<boost::asio::io_service>())
        , m_context(boost::asio::ssl::context::sslv23)
        , m_socket(std::make_unique<sock_t>(*m_io_ptr, m_context))
    {
        logd("Default object created");
    }

    // Create a tcp_client with an external io_service
    tcp_client(boost::asio::io_service &io)
        : m_context(boost::asio::ssl::context::sslv23)
        , m_socket(std::make_unique<sock_t>(io, m_context))
    {
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
    {}

    void operator=(tcp_client &&other)
    {
        m_io_ptr = std::move(other.m_io_ptr);
        m_context = std::move(other.m_context);
        m_socket = std::move(other.m_socket);
    }

    ~tcp_client() = default;

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
    }

    void send(uint16_t type, uint16_t flags = 0, uint32_t data_size = 0,
              std::optional<std::string> data = std::optional<std::string>())
    {
        uint64_t pkt = serialize_packet(type, flags, data_size);
        m_os << pkt;
        if (data)
            m_os << *data;

        boost::asio::async_write(
            m_socket, m_output,
            boost::asio::transfer_exactly(data_size + sizeof(uint64_t)),
            boost::bind(&tcp_client::on_write, shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
    }

  private:
    using tcp = boost::asio::ip::tcp;

    void connect(const std::string &host, const std::string &port)
    {
        m_host = host;
        m_port = port;

        tcp::resolver resolver(m_socket->get_io_service());
        tcp::resolver::query query(host, port);
        resolver.async_resolve(
            query, boost::bind(&tcp_client::on_resolve, shared_from_this(),
                               boost::asio::placeholders::error,
                               boost::asio::placeholders::iterator));
    }

    // boost::asio async callbacks
    void on_resolve(const boost::system::error_code &ec,
                    tcp::resolver::iterator iter)
    {
        if (!ec) {
            logd("resolve succeeded");
            auto ep = *iter++;
            boost::asio::async_connect(
                m_socket->lowest_layer(), ep,
                boost::bind(&tcp_client::on_connect, shared_from_this(),
                            boost::asio::placeholders::error, iter));
            // connect!
        } else {
            loge(ec.message());
        }
    }

    void on_connect(const boost::system::error_code &ec,
                    tcp::resolver::iterator iter)
    {
        if (!ec) {
            logd("connect succeeded");
            m_socket->async_handshake(
                boost::asio::ssl::stream_base::client,
                boost::bind(&tcp_client::on_handshake, shared_from_this(),
                            boost::asio::placeholders::error));
            // handshake
        } else if (iter != tcp::resolver::iterator()) {
            logd("connect failed, trying next address");
            auto ep = *iter++;
            boost::asio::async_connect(
                m_socket->lowest_layer(), ep,
                boost::bind(&tcp_client::on_connect, shared_from_this(),
                            boost::asio::placeholders::error, iter));
        } else {
            loge(ec.message());
        }
    }

    void on_handshake(const boost::system::error_code &ec)
    {
        if (!ec) {
            logd("handshake succeeded");
            boost::asio::async_read(
                m_socket, m_input,
                boost::asio::transfer_exactly(sizeof(uint64_t)),
                boost::bind(&tcp_client::on_read_packet, shared_from_this(),
                            boost::asio::placeholders::error));
        } else {
            loge(ec.message());
        }
    }

    void on_read_packet(const boost::system::error_code &ec)
    {
        if (!ec) {
            uint64_t pkt;
            m_is >> pkt;

            auto [type, flags, size] = deserialize_packet(pkt);
            logd("received packet, type = ", type, ", flags = ", flags,
                 " size = ", size);

            if (size > 0) {
                boost::asio::async_read(
                    m_socket, m_input, boost::asio::transfer_exactly(size),
                    boost::bind(&tcp_client::on_read_data,
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred,
                                pkt));
            } else {
                if (m_read_f) {
                    message msg(pkt, std::nullopt);
                    m_read_f(shared_from_this(), msg);
                }
                boost::asio::async_read(
                    m_socket, m_input,
                    boost::asio::transfer_exactly(sizeof(uint64_t)),
                    boost::bind(&tcp_client::on_read_packet, shared_from_this(),
                                boost::asio::placeholders::error));
            }
        } else {
            loge(ec.message());
        }
    }

    void on_read_data(const boost::system::error_code &ec, std::size_t bytes,
                      uint64_t pkt)
    {
        if (!ec) {
            std::string data(bytes, '0');
            m_is.read(&data[0], bytes);
            logd("received ", bytes, " bytes of data");

            // Construct message, pass to read fn if provided
            if (m_read_f) {
                message msg(pkt, data);
                m_read_f(shared_from_this(), msg);
            }

            boost::asio::async_read(
                m_socket, m_input,
                boost::asio::transfer_exactly(sizeof(uint64_t)),
                boost::bind(&tcp_client::on_read_packet, shared_from_this(),
                            boost::asio::placeholders::error));
        } else {
            loge(ec.message());
        }
    }

    void on_write(const boost::system::error_code &ec, std::size_t bytes)
    {
        if (!ec) {
            logd("sent ", bytes, " bytes of data (", bytes - sizeof(uint64_t),
                 " data size)");
        } else {
            loge(ec.message());
        }
    }

  private:
    boost::asio::streambuf m_input;
    std::istream m_is{&m_input};

    boost::asio::streambuf m_output;
    std::ostream m_os{&m_output};

    std::unique_ptr<boost::asio::io_service> m_io_ptr;

    std::string m_host;
    std::string m_port;

  protected:
    boost::asio::ssl::context m_context;
    std::unique_ptr<sock_t> m_socket;
};

}; // namespace ns

#endif
