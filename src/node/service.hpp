#ifndef NS_NODE_SERVICE_HPP
#define NS_NODE_SERVICE_HPP

#include "server.hpp"

namespace ns
{
namespace node
{

/**
 * A service instance handles local service connections
 * to nodes. These are applications written with the public
 * nswarm SDK that can implement methods or subscribe to events.
 **/
class service : public connection<service>
{
public:
    using connection::connection;
};

class service_server : public tcp_server<service>
{
public:
    using tcp_server::tcp_server;
};

}; // namespace node
}; // namespace ns

#endif
