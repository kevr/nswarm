/**
 * \project nswarm
 * \file async.hpp
 * \description Asynchronous helper objects and classes.
 **/
#ifndef NS_ASYNC_HPP
#define NS_ASYNC_HPP

#include "data.hpp"
#include "logging.hpp"
#include "types.hpp"
#include <algorithm>
#include <bitset>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <functional>
#include <iomanip>
#include <memory>
#include <set>
#include <string>

namespace ns
{

template <typename T>
using async_read_function = std::function<void(std::shared_ptr<T>, data)>;

template <typename T>
using async_connect_function = std::function<void(std::shared_ptr<T>)>;

template <typename T>
using async_close_function = std::function<void(std::shared_ptr<T>)>;

template <typename T>
using async_error_function =
    std::function<void(std::shared_ptr<T>, const boost::system::error_code &)>;

template <typename T>
using async_accept_function = std::function<void(std::shared_ptr<T>)>;

/**
 * \class async_object
 * \brief Provides async callback functions to a derived class
 **/
template <typename T, typename CallbackT = T>
class async_object : public std::enable_shared_from_this<T>
{
public:
    std::shared_ptr<T> on_read(async_read_function<CallbackT> f)
    {
        m_read_f = f;
        return this->shared_from_this();
    }

    std::shared_ptr<T> on_connect(async_connect_function<CallbackT> f)
    {
        m_connect_f = f;
        return this->shared_from_this();
    }

    std::shared_ptr<T> on_close(async_close_function<CallbackT> f)
    {
        m_close_f = f;
        return this->shared_from_this();
    }

    std::shared_ptr<T> on_error(async_error_function<CallbackT> f)
    {
        m_error_f = f;
        return this->shared_from_this();
    }

protected:
    template <typename... Args>
    void call_read(Args &&... args)
    {
        m_read_f(std::forward<Args>(args)...);
    }

    bool has_read() const
    {
        return m_read_f != nullptr;
    }

    template <typename... Args>
    void call_connect(Args &&... args)
    {
        m_connect_f(std::forward<Args>(args)...);
    }

    bool has_connect() const
    {
        return m_connect_f != nullptr;
    }

    template <typename... Args>
    void call_close(Args &&... args)
    {
        m_close_f(std::forward<Args>(args)...);
    }

    bool has_close() const
    {
        return m_close_f != nullptr;
    }

    template <typename... Args>
    void call_error(Args &&... args)
    {
        m_error_f(std::forward<Args>(args)...);
    }

    bool has_error() const
    {
        return m_error_f != nullptr;
    }

private:
    async_read_function<CallbackT> m_read_f;
    async_connect_function<CallbackT> m_connect_f;
    async_close_function<CallbackT> m_close_f;
    async_error_function<CallbackT> m_error_f;
};

/**
 * \class async_io_object
 * \brief Base object for connections (both server and client-side).
 * \description async_io_object provides the entire i/o layer for
 * our connect, read, write, and close actions. This object
 * can be used to implement a functional tcp client or tcp server
 * connection.
 *
 * Example:
 * class A : public async_io_object<A>
 * {
 * public:
 *     void run()
 *     {
 *         on_read([](auto c) {
 *                 // ...
 *             })
 *             ->on_close([](auto c) {
 *                 // ...
 *             });
 *
 *         // Initiate handshake; generally what you'd do
 *         // from a server connection
 *         m_socket->async_handshake(ssl::stream_base::client,
 *             boost::bind(&A::async_on_read, this->shared_from_this(),
 *                 boost::asio::placeholders::error));
 *     }
 * };
 *
 **/
template <typename T>
class async_io_object : public async_object<T>
{
public:
    // IMPORTANT: This function *must* be called from a derived
    // object constructor. m_socket is assumed to be valid
    // after the constructor of any associated object completes.
    void initialize_socket(io_service &io, ssl::context &ctx)
    {
        m_socket = std::make_unique<tcp_socket>(io, ctx);
    }

    void send(const data &data)
    {
        uint64_t pkt = data.packet();
        m_os.write(reinterpret_cast<char *>(&pkt), sizeof(uint64_t));

        if (has_debug_logging()) {
            logd("sending bits: ", std::bitset<sizeof(uint64_t) * 8>(pkt));
        }

        if (data.get_string().size())
            m_os << data.get_string();
        boost::asio::async_write(
            *m_socket, m_output,
            boost::asio::transfer_exactly(data.size() + sizeof(data.packet())),
            boost::bind(&T::async_on_write, this->shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
    }

    void send(uint16_t type, uint16_t flags = 0,
              std::optional<std::string> data = std::optional<std::string>())
    {
        uint32_t data_size = 0; // Assume no data

        // Here: if data is present, serialize_packet with the real data size
        // and set data_size at the same time.
        uint64_t pkt =
            data ? serialize_packet(type, flags, data_size)
                 : serialize_packet(type, flags, (data_size = data->size()));

        m_os << pkt; // Put the packet header into the stream
        if (data)
            m_os << *data;

        boost::asio::async_write(
            *m_socket, m_output,
            boost::asio::transfer_exactly(data_size + sizeof(pkt)),
            boost::bind(&T::async_on_write, this->shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
    }

    tcp_socket &socket()
    {
        return *m_socket;
    }

    const bool connected() const
    {
        return m_socket->lowest_layer().is_open();
    }

protected:
    void start_handshake(ssl::stream_base::handshake_type handshake_type)
    {
        m_socket->async_handshake(
            handshake_type,
            boost::bind(&T::async_on_handshake, this->shared_from_this(),
                        boost::asio::placeholders::error));
    }

    // boost::asio async callbacks
    void async_on_resolve(const boost::system::error_code &ec,
                          tcp::resolver::iterator iter)
    {
        if (!ec) {
            logd("resolve succeeded");
            auto ep = iter++;
            boost::asio::async_connect(
                m_socket->lowest_layer(), ep,
                boost::bind(&T::async_on_connect, this->shared_from_this(),
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
            start_handshake(ssl::stream_base::client);
        } else if (iter != tcp::resolver::iterator()) {
            logd("connect failed, trying next address");
            auto ep = iter++;
            boost::asio::async_connect(
                m_socket->lowest_layer(), ep,
                boost::bind(&T::async_on_connect, this->shared_from_this(),
                            boost::asio::placeholders::error, iter));
        } else {
            handle_error(ec, "unable to connect: ", ec.message());
        }
    }

    void async_on_handshake(const boost::system::error_code &ec)
    {
        if (!ec) {
            logd("handshake succeeded");

            if (this->has_connect())
                this->call_connect(this->shared_from_this());

            start_read();
        } else {
            handle_error(ec, "client socket closed while handshaking");
        }
    }

    void async_on_read_packet(const boost::system::error_code &ec,
                              std::size_t bytes)
    {
        if (!ec) {

            uint64_t pkt = 0;
            m_is.read(reinterpret_cast<char *>(&pkt), sizeof(uint64_t));
            logd("packet received (bits): ",
                 std::bitset<sizeof(uint64_t) * 8>(pkt));

            auto [type, flags, size] = deserialize_packet(pkt);
            logd("deserialized packet: type = ", type, ", flags = ", flags,
                 " size = ", size);

            if (size > 0) {
                boost::asio::async_read(
                    *m_socket, m_input, boost::asio::transfer_exactly(size),
                    boost::bind(
                        &T::async_on_read_data, this->shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred, pkt));
            } else {
                // IMPORTANT: This is all wrong. We need better overrides for
                // ns::data
                if (this->has_read()) {
                    data x(pkt);
                    this->call_read(this->shared_from_this(), x);
                }
                start_read();
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
            if (this->has_read()) {
                class data x(pkt, 0, data);
                this->call_read(this->shared_from_this(), x);
            }
            start_read();
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
            if (this->has_error())
                this->call_error(this->shared_from_this(), ec);
        } else {
            logd(std::forward<Args>(args)..., ": ", ec.message());
            close();
            logd("tcp handle closed: ", std::showbase, std::internal,
                 std::setfill('0'), std::hex, std::setw(4),
                 (unsigned long)&*this->shared_from_this());
            if (this->has_close())
                this->call_close(this->shared_from_this());
        }
    }

    void close()
    {
        if (connected()) {
            boost::system::error_code ec; // No need to check, silently fail
            m_socket->shutdown(ec);       // ssl stream shutdown
        }
    }

private:
    void start_read()
    {
        boost::asio::async_read(
            *m_socket, m_input, boost::asio::transfer_exactly(sizeof(uint64_t)),
            boost::bind(&T::async_on_read_packet, this->shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
    }

protected:
    boost::asio::streambuf m_input;
    std::istream m_is{&m_input};

    boost::asio::streambuf m_output;
    std::ostream m_os{&m_output};

    std::unique_ptr<tcp_socket> m_socket;

    // Error whitelist - when encountering one of these errors,
    // we will gracefully close the socket.
    static inline const std::set<boost::system::error_code> m_errors_wl{
        boost::asio::ssl::error::stream_truncated,
        boost::asio::error::operation_aborted,
        boost::asio::error::connection_aborted,
        boost::asio::error::connection_reset,
        boost::asio::error::connection_refused,
        boost::asio::error::eof};
};

}; // namespace ns

#endif
