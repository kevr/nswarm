#ifndef NS_HOST_DAEMON_HPP
#define NS_HOST_DAEMON_HPP

#include "host/node_server.hpp"

#include <nswarm/logging.hpp>

namespace ns
{
namespace host
{

class daemon
{
    io_service m_io;

    // api_server m_api_server{m_io, 6667};
    node_server m_node_server{m_io, 6666};

public:
    // api_server &get_api_server();
    void use_api_certificate(const std::string &cert, const std::string &key);
    void use_api_auth_key(const std::string &key);

    node_server &get_node_server()
    {
        return m_node_server;
    }

    void use_node_certificate(const std::string &cert, const std::string &key)
    {
        m_node_server.use_certificate(cert, key);
    }

    void use_node_auth_key(const std::string &key)
    {
        m_node_server.set_auth_key(key);
    }

    void run()
    {
        m_node_server.run();
        logd("node server running");

        // Run shared io_service
        m_io.run();
    }
};

}; // namespace host
}; // namespace ns

#endif // NS_HOST_DAEMON_HPP
