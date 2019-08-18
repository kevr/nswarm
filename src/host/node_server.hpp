/**
 * \prject nswarm
 * \file host/node_server.hpp
 * \brief Node connection collelctions and tcp server tools.
 *
 * Copyright (c) 2019 Kevin Morris
 * All Rights Reserved.
 **/
#ifndef NS_HOST_NODE_SERVER_HPP
#define NS_HOST_NODE_SERVER_HPP

#include "server.hpp"
#include <nswarm/auth.hpp>
#include <nswarm/protocol.hpp>
#include <set>
#include <string>

namespace ns
{
namespace host
{

class node_connection : public connection<node_connection>,
                        public auth_context<authentication::plain>
{
public:
    using connection::connection;
    virtual ~node_connection() = default;

    node_connection &set_auth_key(const std::string &key)
    {
        auth_context<authentication::plain>::operator=(
            auth_context<authentication::plain>(key));
        return *this;
    }

private:
    set_log_address;
};

class node_server : public tcp_server<node_connection>,
                    protected protocol<node_connection, json_message>
{
public:
    using tcp_server::tcp_server;

    node_server(unsigned short port)
        : tcp_server(port)
    {
        init();
    }

    node_server(io_service &io, unsigned short port)
        : tcp_server(io, port)
    {
        init();
    }

    node_server &set_auth_key(const std::string &key)
    {
        m_auth = auth_context<authentication::plain>(key);
        logi("set auth key to: ", key);
        return *this;
    }

    void init()
    {
        // Bind L0 protocol callbacks
        m_proto
            // Auth is the first stage we begin considering a node
            // for real communication (if it succeeds).
            .on_auth([this](auto node, auto msg) -> void {
                logd("received auth request: ", msg.get_string());

                auto auth_data = net::auth(msg);
                bool authenticated = false;
                try {
                    authenticated = node->authenticate(auth_data.key());
                    if (authenticated) {
                        logi("node authenticated from ", node->remote_host(),
                             ":", node->remote_port());
                    }
                } catch (ns::json::exception &e) {
                    loge("json parse error: ", e.what());
                } catch (std::exception &e) {
                    loge("error while parsing auth request: ", e.what());
                }

                auth_data.update_action(net::action::type::response);
                auth_data.set_authenticated(authenticated);
                node->send(std::move(auth_data));

                if (!authenticated)
                    node->close();
                else
                    // Forward node and json data to L1 auth
                    call_auth(std::move(node), std::move(msg));
            })
            .on_implement([this](auto node, auto msg) {
                if (!node->authenticated()) {
                    loge("client not authenticated during on_subscribe");
                    node->close();
                } else {
                    logd("received implement request: ", msg.get_string());
                    call_implement(std::move(node), std::move(msg));
                }
            })
            .on_subscribe([this](auto node, auto msg) {
                if (!node->authenticated()) {
                    loge("client not authenticated during on_subscribe");
                    node->close();
                } else {
                    logd("received subscribe request: ", msg.get_string());
                    call_subscribe(std::move(node), std::move(msg));
                }
            })
            .on_task([this](auto node, auto msg) {
                if (!node->authenticated()) {
                    loge("client not authenticated during on_task");
                    node->close();
                } else {
                    logd("received task response: ", msg.get_string());
                    call_task(std::move(node), std::move(msg));
                }
            });

        // Bind socket i/o callbacks
        on_accept([this](auto node) {
            m_nodes.emplace(node);
            node->set_auth_key(m_auth.key());
            node->run();
        })
            .on_connect([this](auto node) {
                logi("node connected from ", node->remote_host(), ":",
                     node->remote_port());
            })
            .on_read([this](auto node, auto msg) {
                // When we read from a node, use m_proto to decide what to do.i
                try {
                    m_proto.call(msg.get_type(), node, msg);
                } catch (std::exception &e) {
                    auto type = ns::data_value_string(msg.get_type());
                    loge(
                        "exception thrown while calling protocol method type [",
                        type, "]: ", e.what());
                }
            })
            .on_close([this](auto node) {
                m_nodes.erase(m_nodes.find(node));
                call_removed(node);
                logi("node from ", node->remote_host(), ":",
                     node->remote_port(),
                     " disconnected, removing it from the swarm");
            })
            .on_error([this](auto node, const auto &ec) {
                m_nodes.erase(m_nodes.find(node));
                call_removed(node);
                loge("node removed from the swarm due to: ", ec.message());
            })
            .on_server_error([this](auto server, const auto &ec) {
                loge("node_server had an error: ", ec.message(),
                     ", closing all connections and discontinuing");
                // Close all node connections
                for (auto &node : m_nodes)
                    node->close();
                m_nodes.clear();
            });
    }

    // Publicly accessible callbacks
    using protocol::on_auth;      // action::type::request
    using protocol::on_implement; // action::type::request
    using protocol::on_subscribe; // action::type::request
    using protocol::on_task;      // action::type::response
    using tcp_server::on_removed;

protected:
    // These callbacks are privately handled within
    // node_server::init().
    using tcp_server::on_accept;
    using tcp_server::on_close;
    using tcp_server::on_error;
    using tcp_server::on_read;

private:
    // A set of nodes that we keep track of.
    std::set<std::shared_ptr<node_connection>> m_nodes;

    // A context which we use to compare incoming keys
    // It's possible that we could remove this altogether
    // or wrap it in a simpler set of classes.
    auth_context<authentication::plain> m_auth;

    // L0 protocol handler. This provides a low level handler
    // that we can bind to the client, which we can also call
    // L1 protocol handlers from.
    // The L1 protocol handler is built into this class, inherited
    // via protocol<...>. So, users will bind L1 handlers externally,
    // and we will bind L0 handlers internally.
    //
    protocol<node_connection, json_message> m_proto;

private:
    set_log_address;
}; // namespace host

}; // namespace host
}; // namespace ns

#endif // NS_HOST_NODE_SERVER_HPP
