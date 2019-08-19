#ifndef NS_HOST_DAEMON_HPP
#define NS_HOST_DAEMON_HPP

#include "host/api_server.hpp"
#include "host/node_server.hpp"
#include "manager.hpp"

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
    api_server m_api_server{m_io, 6667};

    // Connection managers for each server
    manager<node_connection, json_message> m_nodes;
    manager<api_connection, json_message> m_users;

    // task_id => api_connection
    using api_connection_ptr = std::shared_ptr<api_connection>;
    std::map<std::string, api_connection_ptr> m_tasks;

public:
    daemon()
    {
        init();
    }

    daemon(unsigned short api_port, unsigned short node_port)
        : m_node_server(m_io, node_port)
        , m_api_server(m_io, api_port)
    {
        init();
    }

    io_service &get_io_service()
    {
        return m_io;
    }

    api_server &get_api_server()
    {
        return m_api_server;
    }

    // api_server &get_api_server();
    void set_api_certificate(const std::string &cert, const std::string &key)
    {
        m_api_server.use_certificate(cert, key);
    }

    void set_api_auth_key(const std::string &key)
    {
        m_api_server.set_auth_key(key);
    }

    node_server &get_node_server()
    {
        return m_node_server;
    }

    void set_node_certificate(const std::string &cert, const std::string &key)
    {
        m_node_server.use_certificate(cert, key);
    }

    void set_node_auth_key(const std::string &key)
    {
        m_node_server.set_auth_key(key);
    }

    int run()
    {
        m_node_server.run();
        m_api_server.run();

        // Run the shared io_service
        m_io.run();
        return 0;
    }

private:
    void init()
    {
        m_node_server.on_auth([](auto node, auto msg) {
            logi("node authenticated and is added to the cluster from ",
                 node->remote_host(), ":", node->remote_port());
        });

        // Figure out how to configure managing nodes in the node server.
        m_node_server.on_removed([this](auto node) {
            logi("node_connection was removed");
        });
    }
};

}; // namespace host
}; // namespace ns

#endif // NS_HOST_DAEMON_HPP
