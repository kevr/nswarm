/**
 * \prject nswarm
 * \file host/node_server.hpp
 * \brief Node connection collelctions and tcp server tools.
 *
 * Copyright (c) 2019 Kevin Morris
 * All Rights Reserved.
 **/
#ifndef NS_HOST_NODE_SERVER
#define NS_HOST_NODE_SERVER

#include "protocol.hpp"
#include "server.hpp"
#include <set>
#include <string>

namespace ns
{
namespace host
{

class node_connection : public connection<node_connection>
{
public:
    using connection::connection;

    void authenticate()
    {
        m_auth = true;
    }

    bool authenticated() const
    {
        return m_auth;
    }

private:
    bool m_auth = false;
};

class node_server : public tcp_server<node_connection>
{
public:
    node_server(io_service &io, unsigned short port)
        : tcp_server(io, port)
    {
        // Bind protocol callbacks
        m_proto
            .on_auth([this](auto c, auto msg) {
                c->authenticate();
                logi("on_auth invoked, authenticated");
            })
            .on_provide([this](auto c, auto msg) {
                if (!c->authenticated()) {
                    loge("client not authenticated during on_subscribe");
                } else {
                    // provide method
                }
            })
            .on_subscribe([this](auto c, auto msg) {
                if (!c->authenticated()) {
                    loge("client not authenticated during on_subscribe");
                } else {
                    // subscribe to event
                }
            })
            .on_task([this](auto c, auto msg) {
                if (!c->authenticated()) {
                    loge("client not authenticated during on_task");
                } else {
                    // respond to task request
                }
            });

        // Bind socket i/o callbacks
        on_accept([this](auto client) { m_nodes.emplace(client); })
            .on_read([this](auto client, auto msg) {
                // When we read from a node, use m_proto to decide what to do.
                try {
                    m_proto.call(static_cast<ns::data_type>(msg.type()), client,
                                 msg);
                } catch (std::exception &e) {
                    loge("exception thrown while calling protocol method type ",
                         msg.type(), ": ", e.what());
                }
            })
            .on_close([this](auto client) {
                m_nodes.erase(m_nodes.find(client));
                logi("node disconnected from the swarm");
            })
            .on_error([this](auto client, const auto &ec) {
                m_nodes.erase(m_nodes.find(client));
                loge("node removed from the swarm due to: ", ec.message());
            })
            .on_server_error([this](auto server, const auto &ec) {
                loge("node_server had an error: ", ec.message());
                loge("closing all node_server connections");
                // Close all node connections
                for (auto &node : m_nodes)
                    node->close();
                // Race here..?
            });
    }

private:
    std::set<std::shared_ptr<node_connection>> m_nodes;

    protocol<node_connection, data> m_proto;
};

}; // namespace host
}; // namespace ns

#endif // NS_HOST_NODE_SERVER
