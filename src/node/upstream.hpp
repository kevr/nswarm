/**
 * \prject nswarm
 * \file node/upstream.hpp
 * \brief nswarm-host node client. Upstream for nswarm-node.
 *
 * Copyright (c) 2019 Kevin Morris
 * All Rights Reserved.
 **/
#ifndef NS_NODE_UPSTREAM_HPP
#define NS_NODE_UPSTREAM_HPP

#include <atomic>
#include <nswarm/auth.hpp>
#include <nswarm/client.hpp>
#include <nswarm/data.hpp>
#include <nswarm/implement.hpp>
#include <nswarm/protocol.hpp>
#include <nswarm/subscribe.hpp>
#include <nswarm/task.hpp>

namespace ns
{
namespace node
{

class upstream : public client<upstream>,
                 public protocol<upstream, json_message>
{
public:
    // We require upstream to be bound to the service server's io
    // so that this connection interleaves with the main executor
    // in the daemon. Creating a separate io_service thread
    // would be overkill.
    upstream(io_service &io)
        : client(io)
    {
        init();
    }

    virtual ~upstream() = default;

    void auth(const std::string &key)
    {
        send(net::make_auth_request(key));
    }

    // We should do exactly what we did to net::auth to the rest
    // of the structures we support below.
    void implement(const std::string &method)
    {
        // implementation takes a method
        send(net::make_impl_request(method));
    }

    void subscribe(const std::string &event)
    {
        send(net::make_subscription_request(event));
    }

    template <typename T>
    void respond(const std::string &task_id, net::task::type task_t,
                 const T &response)
    {
        json js;
        js["task_id"] = task_id;
        js["data"] = response;

        auto data = match(net::task::deduce(task_t), [&](auto e) {
            return net::make_task_response<decltype(e)::type>(task_id);
        });
        data.update(js);
        send(std::move(data));
    }

    const bool authenticated() const
    {
        return m_is_authenticated.load();
    }

private:
    void init()
    {
        on_heartbeat([this](auto c, auto msg) {
            auto hb = net::make_heartbeat_response();
            c->send(hb);
        });

        // Setup protocol callbacks
        m_proto
            .on_auth([this](auto client, auto message) {
                auto json = message.get_json();
                if (json.at("data")) {
                    m_is_authenticated.exchange(true);
                    call_auth(client, message);
                    logi("authenticated with upstream host");
                } else {
                    m_is_authenticated.exchange(false);
                    this->close();
                }
            })
            .on_implement([this](auto client, auto message) {
                logi("on_implement response received");
                call_implement(client, message);
            })
            .on_subscribe([this](auto client, auto message) {
                logi("on_subscribe response received");
                call_subscribe(client, message);
            })
            .on_task([this](auto client, auto message) {
                logi("on_task request received: ", message);
                call_task(client, message);
            })
            .on_heartbeat([this](auto client, auto message) {
                logi("on_heartbeat request received");
                message.update_action(net::action::type::response);
                client->send(message);
            });

        on_connect([this](auto client) {
            logi("upstream connected (remote = ", client->remote_host(), ":",
                 client->remote_port(), ")");
        })
            .on_read([this](auto client, auto msg) {
                try {
                    m_proto.call(msg.get_type(), client, msg);
                } catch (std::exception &e) {
                    loge(e.what());
                }
            })
            .on_close([this](auto client) {
                // Automatically try to reconnect
                logd("upstream connection closed, reconnecting in 10 seconds");
                logi("upstream connection was closed, attempting to reconnect "
                     "in 10 seconds");
                std::this_thread::sleep_for(std::chrono::seconds(10));
                client->run(client->remote_host(), client->remote_port());
            })
            .on_error([this](auto client, const auto &ec) {
                logd("socket error: ", ec.message());
                logd("upstream connection closed, reconnecting in 10 seconds");
                std::this_thread::sleep_for(std::chrono::seconds(10));
                client->run(client->remote_host(), client->remote_port());
            });
        ;
    }

protected:
    using protocol::on_heartbeat;

private:
    std::atomic<bool> m_is_authenticated{false};

    // L0 protocol handlers.
    protocol<upstream, json_message> m_proto;

private:
    set_log_address;

}; // class upstream

}; // namespace node
}; // namespace ns

#endif // NS_NODE_UPSTREAM_HPP
