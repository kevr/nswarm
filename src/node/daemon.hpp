#ifndef NS_NODE_DAEMON_HPP
#define NS_NODE_DAEMON_HPP

#include "manager.hpp"
#include "node/upstream.hpp"

#include <nswarm/logging.hpp>

namespace ns
{
namespace node
{

class daemon
{
    io_service m_io;

    // Service server

    // Default to upstream bound to m_io
    std::shared_ptr<upstream> m_upstream{std::make_shared<upstream>(m_io)};

    std::string m_upstream_host;
    std::string m_upstream_port{"6666"};

    // Upstream auth key
    std::string m_upstream_key;

public:
    daemon()
    {
        init();
    }

    daemon(std::string upstream_host, std::string upstream_port)
    {
        set_upstream(std::move(upstream_host), std::move(upstream_port));
        init();
    }

    void set_upstream(std::string host, std::string port)
    {
        m_upstream_host = std::move(host);
        m_upstream_port = std::move(port);
    }

    void set_upstream_auth_key(const std::string &key)
    {
        m_upstream_key = key;
    }

    io_service &get_io_service()
    {
        return m_io;
    }

    int run()
    {
        // Begin connecting to upstream.
        m_upstream->run(m_upstream_host, m_upstream_port);

        // Run the shared io_service
        m_io.run();
        return 0;
    }

private:
    void init()
    {
        // For our upstream network flow: We want to authenticate
        // on connecting, then continue to relay protocol messages
        // back and forth between node services and upstream.
        m_upstream
            ->on_connect([this](auto c) {
                logi("upstream connected to ", c->remote_host(), ":",
                     c->remote_port(), ", authenticating");
                c->auth(m_upstream_key);
            })
            // on_auth, on_implement, and on_subscribe should verify
            // that they were successful (they will be responses)
            // on_task should proxy data onto registered services on this node.
            .on_auth([](auto c, auto msg) {
                logi("upstream authenticated");
            })
            .on_implement([](auto c, auto msg) {
                // Send implement data back to original service
            })
            .on_subscribe([](auto c, auto msg) {
                // Send subscribe data back to original service
            })
            .on_task([](auto c, auto msg) {
                // Send task request to services
            });
    }
};

}; // namespace node
}; // namespace ns

#endif // NS_NODE_DAEMON_HPP
