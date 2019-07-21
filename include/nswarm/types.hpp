#ifndef NS_TYPES_HPP
#define NS_TYPES_HPP

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

namespace ns
{
using io_service = boost::asio::io_service;
using tcp = boost::asio::ip::tcp;
namespace ssl = boost::asio::ssl;

using tcp_socket = ssl::stream<tcp::socket>;

}; // namespace ns

#endif
