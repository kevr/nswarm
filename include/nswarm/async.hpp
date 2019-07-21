/**
 * \project nswarm
 * \file async.hpp
 * \description Asynchronous helper objects and classes.
 **/
#ifndef NS_ASYNC_HPP
#define NS_ASYNC_HPP

#include <nswarm/data.hpp>
#include <nswarm/logging.hpp>
#include <nswarm/types.hpp>
#include <algorithm>
#include <bitset>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <functional>
#include <iomanip>
#include <memory>
#include <set>
#include <stdexcept>
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

/**
 * \class async_object
 * \brief Provides async callback functions to a derived class
 **/
// T = connection type
// U = object type (derivative)
template <typename Derivative, typename T = Derivative>
class async_object
{
public:
    async_object() = default;
    virtual ~async_object() = default;

    Derivative &on_read(async_read_function<T> f)
    {
        m_read_f = f;
        return *reinterpret_cast<Derivative *>(this);
    }

    Derivative &on_connect(async_connect_function<T> f)
    {
        m_connect_f = f;
        return *reinterpret_cast<Derivative *>(this);
    }

    Derivative &on_close(async_close_function<T> f)
    {
        m_close_f = f;
        return *reinterpret_cast<Derivative *>(this);
    }

    Derivative &on_error(async_error_function<T> f)
    {
        m_error_f = f;
        return *reinterpret_cast<Derivative *>(this);
    }

protected:
    template <typename... Args>
    void call_read(Args &&... args) noexcept
    {
        m_read_f(std::forward<Args>(args)...);
    }

    bool has_read() const noexcept
    {
        return m_read_f != nullptr;
    }

    template <typename... Args>
    void call_connect(Args &&... args) noexcept
    {
        m_connect_f(std::forward<Args>(args)...);
    }

    bool has_connect() const noexcept
    {
        return m_connect_f != nullptr;
    }

    template <typename... Args>
    void call_close(Args &&... args) noexcept
    {
        m_close_f(std::forward<Args>(args)...);
    }

    bool has_close() const noexcept
    {
        return m_close_f != nullptr;
    }

    template <typename... Args>
    void call_error(Args &&... args) noexcept
    {
        m_error_f(std::forward<Args>(args)...);
    }

    bool has_error() const noexcept
    {
        return m_error_f != nullptr;
    }

private:
    async_read_function<T> m_read_f;
    async_connect_function<T> m_connect_f;
    async_close_function<T> m_close_f;
    async_error_function<T> m_error_f;

protected:
    set_log_address;
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
class async_io_object : public async_object<T>,
                        public std::enable_shared_from_this<T>
{
public:
    // IMPORTANT: This function *must* be called from a derived
    // object constructor. m_socket is assumed to be valid
    // after the constructor of any associated object completes.
    void initialize_socket(io_service &io, ssl::context &ctx)
    {
        m_socket = std::make_unique<tcp_socket>(io, ctx);
    }

    template <typename U>
    void send(const basic_data<U> &data)
    {
        const auto &str = data.get_string();

        // payload size cannot be larger than our data_size segment of header
        if (str.size() > std::numeric_limits<uint32_t>::max())
            throw std::out_of_range(
                "data.size() is too large. maximum payload size is "
                "std::numeric_limits<uint32_t>::max(): " +
                std::to_string(std::numeric_limits<uint32_t>::max()));

        // data.size() *must* be equivalent to payload size
        if (data.size() != str.size())
            throw std::invalid_argument(
                "data_size in header mismatched string data size: " +
                std::to_string(data.size()) + " vs " +
                std::to_string(data.get_string().size()));

        // We should send a valid header no matter what.
        uint64_t header = data.header();
        m_os.write(reinterpret_cast<char *>(&header), sizeof(uint64_t));

        // If data was provided, write it to the stream
        if (data.size()) {
            m_os.write(str.data(), data.size());
        }

        boost::asio::async_write(
            *m_socket, m_output,
            boost::asio::transfer_exactly(data.size() + sizeof(data.header())),
            boost::bind(&T::async_on_write, this->shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
    }

    void send(uint16_t type, uint16_t flags = 0,
              std::optional<std::string> data = std::optional<std::string>())
    {
        uint32_t data_size = 0; // Assume no data

        // Here: if data is present, serialize_header with the real data size
        // and set data_size at the same time.
        uint64_t pkt =
            data ? serialize_header(type, flags, data_size)
                 : serialize_header(type, flags, (data_size = data->size()));

        m_os << pkt; // Put the header header into the stream
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

    const std::string &remote_host() const
    {
        return m_remote_host;
    }

    const std::string &remote_port() const
    {
        return m_remote_port;
    }

protected:
    void
    start_handshake(ssl::stream_base::handshake_type handshake_type) noexcept
    {
        m_socket->async_handshake(
            handshake_type,
            boost::bind(&T::async_on_handshake, this->shared_from_this(),
                        boost::asio::placeholders::error));
    }

    void start_resolve(tcp::resolver &resolver, const std::string &host,
                       const std::string &port) noexcept
    {
        logd("resolving ", host, ":", port);
        tcp::resolver::query query(host, port);
        resolver.async_resolve(
            query, boost::bind(&T::async_on_resolve, this->shared_from_this(),
                               boost::asio::placeholders::error,
                               boost::asio::placeholders::iterator));
    }

    template <typename... Args>
    void handle_error(const boost::system::error_code &ec,
                      Args &&... args) noexcept
    {
        // If the error is not in the whitelist, explicitly log it out
        if (m_errors_wl.find(ec) == m_errors_wl.end()) {
            loge(ec.message());
            close();
            logd("calling on_error");
            if (this->has_error())
                this->call_error(this->shared_from_this(), ec);
        } else {
            logd(std::forward<Args>(args)..., ": ", ec.message());
            close();
            logd("calling on_close");
            if (this->has_close())
                this->call_close(this->shared_from_this());
        }
    }

    void close() noexcept
    {
        logd("close called, dispatching to io_service");
        boost::system::error_code ec; // No need to check, silently fail
        m_socket->shutdown(ec);       // ssl stream shutdown
        m_socket->lowest_layer().close(ec);
    }

    void set_endpoint(const tcp::endpoint &ep)
    {
        m_remote_host = ep.address().to_v4().to_string();
        m_remote_port = std::to_string(ep.port());
    }

private:
    // boost::asio async callbacks
    void async_on_resolve(const boost::system::error_code &ec,
                          tcp::resolver::iterator iter) noexcept
    {
        trace();

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
                          tcp::resolver::iterator next) noexcept
    {
        trace();

        // This will always be the same value and never modified.
        static const auto end = tcp::resolver::iterator();

        if (!ec) {
            logd("connect succeeded");
            start_handshake(ssl::stream_base::client);
        } else if (next != end) {
            logd("connect failed, trying next address");
            auto ep = next++;
            boost::asio::async_connect(
                m_socket->lowest_layer(), ep,
                boost::bind(&T::async_on_connect, this->shared_from_this(),
                            boost::asio::placeholders::error, next));
        } else {
            handle_error(ec, "unable to connect: ", ec.message());
        }
    }

    void async_on_handshake(const boost::system::error_code &ec) noexcept
    {
        trace();

        if (!ec) {
            logd("handshake succeeded");

            set_endpoint(socket().lowest_layer().remote_endpoint());

            if (this->has_connect())
                this->call_connect(this->shared_from_this());

            start_read();
        } else {
            handle_error(ec, "client socket closed while handshaking");
        }
    }

    void async_on_read_header(const boost::system::error_code &ec,
                              std::size_t bytes) noexcept
    {
        trace();

        if (!ec) {

            data x;
            x.read_header(m_is);

            logd("header received: ",
                 std::bitset<sizeof(uint64_t) * 8>(x.header()));

            auto type = ns::data_value_string(x.type());
            auto direction = ns::action_value_string(x.direction());
            logd("deserialized header: type = ", type,
                 ", params = ", x.params(), ", direction = ", direction,
                 ", size = ", x.size());

            if (x.size() > 0) {
                boost::asio::async_read(
                    *m_socket, m_input, boost::asio::transfer_exactly(x.size()),
                    boost::bind(&T::async_on_read_data,
                                this->shared_from_this(),
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred,
                                std::move(x)));
            } else {
                // IMPORTANT: This is all wrong. We need better overrides for
                // ns::data
                if (this->has_read()) {
                    this->call_read(this->shared_from_this(), std::move(x));
                }
                start_read();
            }
        } else {
            handle_error(ec, "client socket closed while reading data header");
        }
    }

    void async_on_read_data(const boost::system::error_code &ec,
                            std::size_t bytes, data x) noexcept
    {
        trace();

        if (!ec) {
            x.read_data(m_is);
            if (this->has_read()) {
                this->call_read(this->shared_from_this(), std::move(x));
            }

            // Start reading again
            start_read();
        } else {
            handle_error(ec, "client socket closed while reading data chunk");
        }
    }

    void async_on_write(const boost::system::error_code &ec,
                        std::size_t bytes) noexcept
    {
        trace();

        if (!ec) {
            logd("sent ", bytes, " bytes of data (", bytes - sizeof(uint64_t),
                 " data size)");
        } else {
            handle_error(ec, "client socket closed while writing");
        }
    }

    void start_read() noexcept
    {
        boost::asio::async_read(
            *m_socket, m_input, boost::asio::transfer_exactly(sizeof(uint64_t)),
            boost::bind(&T::async_on_read_header, this->shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
    }

protected:
    std::unique_ptr<tcp_socket> &socket_ptr()
    {
        return m_socket;
    }

private:
    boost::asio::streambuf m_input;
    std::istream m_is{&m_input};

    boost::asio::streambuf m_output;
    std::ostream m_os{&m_output};

    std::unique_ptr<tcp_socket> m_socket;

    std::string m_remote_host;
    std::string m_remote_port;

    // Error whitelist - when encountering one of these errors,
    // we will gracefully close the socket.
    static inline const std::set<boost::system::error_code> m_errors_wl{
        boost::asio::ssl::error::stream_truncated,
        boost::asio::error::operation_aborted,
        boost::asio::error::connection_aborted,
        boost::asio::error::connection_reset,
        boost::asio::error::connection_refused,
        boost::asio::error::eof};

    set_log_address;
};

}; // namespace ns

#endif
