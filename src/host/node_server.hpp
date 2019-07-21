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
                    protected protocol<node_connection, ns::data>
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
        // Bind protocol callbacks
        on_auth([this](auto c, auto msg) -> void {
            // This is really fucked up. We need to fix this data type mess.
            logi("on_auth invoked, authenticating against: ", msg.get_string());
            auth_request<auth_type::key> auth;

            bool authenticated = false;
            ns::json json;
            try {
                auth = make_data<auth_request<auth_type::key>>(msg);
                authenticated = c->authenticate(auth.key());
                json = ns::json(auth.get_json());
            } catch (ns::json::exception &e) {
                loge("json parse error: ", e.what());
            } catch (std::exception &e) {
                loge("error while parsing auth request: ", e.what());
            }

            json["data"] = authenticated;

            c->send(ns::auth_response<auth_type::key>(json.dump()));
            if (!authenticated)
                c->close();
        })
            .on_provide([this](auto c, auto msg) {
                if (!c->authenticated()) {
                    loge("client not authenticated during on_subscribe");
                    c->close();
                } else {
                    logd("received provide: ", msg.get_string());
                }
            })
            .on_subscribe([this](auto c, auto msg) {
                if (!c->authenticated()) {
                    loge("client not authenticated during on_subscribe");
                    c->close();
                } else {
                    logd("received subscribe: ", msg.get_string());
                }
            })
            .on_task([this](auto c, auto msg) {
                if (!c->authenticated()) {
                    loge("client not authenticated during on_task");
                    c->close();
                } else {
                    logd("received task: ", msg.get_string());
                }
            });

        // Bind socket i/o callbacks
        on_accept([this](auto client) {
            m_nodes.emplace(client);
            client->set_auth_key(m_auth.key());
            client->run();

            logi("node connected to the swarm");
        })
            .on_read([this](auto client, auto msg) {
                // When we read from a node, use m_proto to decide what to do.i
                // try {
                this->call(msg.type(), client, msg);
                /*
            } catch (std::exception &e) {
                auto type = ns::data_value_string(msg.type());
                loge(
                    "exception thrown while calling protocol method type [",
                    type, "]: ", e.what());
            }
            */
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
                loge("node_server had an error: ", ec.message(),
                     ", closing all connections and discontinuing");
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
