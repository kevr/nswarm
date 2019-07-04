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
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <functional>
#include <memory>
#include <set>
#include <string>

namespace ns
{

template <typename T>
using async_read_function =
    std::function<void(std::shared_ptr<T>, const message &)>;

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
    async_object &on_read(async_read_function<CallbackT> f)
    {
        m_read_f = f;
        return *this;
    }

    async_object &on_connect(async_connect_function<CallbackT> f)
    {
        m_connect_f = f;
        return *this;
    }

    async_object &on_close(async_close_function<CallbackT> f)
    {
        m_close_f = f;
        return *this;
    }

    async_object &on_error(async_error_function<CallbackT> f)
    {
        m_error_f = f;
        return *this;
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
 **/
template <typename T>
class async_io_object : public async_object<T>
{
public:
    void send(uint16_t type, uint16_t flags = 0,
              std::optional<std::string> data = std::optional<std::string>())
    {
        uint32_t data_size = data ? (*data).size() : 0;
        uint64_t pkt = serialize_packet(type, flags, data_size);
        m_os << pkt;
        if (data)
            m_os << *data;

        boost::asio::async_write(
            *m_socket, m_output,
            boost::asio::transfer_exactly(data_size + sizeof(uint64_t)),
            boost::bind(&T::async_on_write, this->shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
    }

protected:
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
            m_socket->async_handshake(
                boost::asio::ssl::stream_base::client,
                boost::bind(&T::async_on_handshake, this->shared_from_this(),
                            boost::asio::placeholders::error));
            // handshake
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

            boost::asio::async_read(
                *m_socket, m_input,
                boost::asio::transfer_exactly(sizeof(uint64_t)),
                boost::bind(&T::async_on_read_packet, this->shared_from_this(),
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
                    boost::bind(
                        &T::async_on_read_data, this->shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred, pkt));
            } else {
                if (this->has_read()) {
                    message msg(pkt, std::nullopt);
                    this->call_read(this->shared_from_this(), msg);
                }
                boost::asio::async_read(
                    *m_socket, m_input,
                    boost::asio::transfer_exactly(sizeof(uint64_t)),
                    boost::bind(&T::async_on_read_packet,
                                this->shared_from_this(),
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
            if (this->has_read()) {
                message msg(pkt, data);
                this->call_read(this->shared_from_this(), msg);
            }

            boost::asio::async_read(
                *m_socket, m_input,
                boost::asio::transfer_exactly(sizeof(uint64_t)),
                boost::bind(&T::async_on_read_packet, this->shared_from_this(),
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
            if (this->has_error())
                this->call_error(this->shared_from_this(), ec);
        } else {
            logd(std::forward<Args>(args)..., ": ", ec.message());
            close();
            if (this->has_close())
                this->call_close(this->shared_from_this());
        }
    }

    void close()
    {
        boost::system::error_code ec; // No need to check, silently fail
        m_socket->shutdown(ec);       // ssl stream shutdown
    }

protected:
    boost::asio::streambuf m_input;
    std::istream m_is{&m_input};

    boost::asio::streambuf m_output;
    std::ostream m_os{&m_output};

    std::unique_ptr<socket> m_socket;

    // Error whitelist - when encountering one of these errors,
    // we will gracefully close the socket.
    static inline const std::set<boost::system::error_code> m_errors_wl{
        boost::asio::ssl::error::stream_truncated,
        boost::asio::error::operation_aborted,
        boost::asio::error::connection_aborted,
        boost::asio::error::connection_reset,
        boost::asio::error::connection_refused};
};

}; // namespace ns

#endif
