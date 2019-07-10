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

#include "auth.hpp"
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

    virtual ~node_connection() = default;

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

    set_log_address;
};

class node_server : public tcp_server<node_connection>,
                    protected protocol<node_connection, ns::data>
{
public:
    using tcp_server::tcp_server;

    node_server(unsigned short port)
        : tcp_server(port)
        , m_auth("")
    {
        init();
    }

    node_server(io_service &io, unsigned short port)
        : tcp_server(io, port)
        , m_auth("")
    {
        init();
    }

    void init()
    {
        // Bind protocol callbacks
        on_auth([this](auto c, auto msg) {
            logi("on_auth invoked, authenticating against: ", msg.get_string());

            auto json = msg.get_json();
            json["data"] = m_auth.authenticate("");

            auto json_str = json.dump();
            ns::data data(serialize_header(msg.type(), action_type::response,
                                           json_str.size()),
                          json_str);
            c->send(data);
        })
            .on_provide([this](auto c, auto msg) {
                if (!c->authenticated()) {
                    loge("client not authenticated during on_subscribe");
                    c->close();
                } else {
                    // provide method
                }
            })
            .on_subscribe([this](auto c, auto msg) {
                if (!c->authenticated()) {
                    loge("client not authenticated during on_subscribe");
                    c->close();
                } else {
                    // subscribe to event
                }
            })
            .on_task([this](auto c, auto msg) {
                if (!c->authenticated()) {
                    loge("client not authenticated during on_task");
                    c->close();
                } else {
                    // respond to task response
                }
            });

        // Bind socket i/o callbacks
        on_accept([this](auto client) {
            m_nodes.emplace(client);
            client->run();

            logi("node connected to the swarm");
        })
            .on_read([this](auto client, auto msg) {
                // When we read from a node, use m_proto to decide what to do.
                try {
                    this->call(msg.type(), client, msg);
                } catch (std::exception &e) {
                    auto type = ns::data_type_string(msg.type());
                    loge(
                        "exception thrown while calling protocol method type [",
                        type, "]: ", e.what());
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

protected:
    using tcp_server::on_accept;
    using tcp_server::on_close;
    using tcp_server::on_error;
    using tcp_server::on_read;

private:
    std::set<std::shared_ptr<node_connection>> m_nodes;
    auth_context<authentication::plain> m_auth;

    set_log_address;
}; // namespace host

}; // namespace host
}; // namespace ns

#endif // NS_HOST_NODE_SERVER
