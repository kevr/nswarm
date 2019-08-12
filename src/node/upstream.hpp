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
                 protected protocol<upstream, json_message>
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

    //
    // Data structure:
    //
    // { "task_id": "1234abcd", "data": T }
    //
    // Examples
    //
    // respond("1234", std::vector<std::string>{"a", "b"});
    // respond("1234", 123);
    // respond("1234", "hello!");
    //
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

    virtual ~upstream() = default;

    const bool authenticated() const
    {
        return m_is_authenticated;
    }

private:
    void init()
    {
        on_heartbeat([this](auto c, auto msg) {
            auto hb = net::make_heartbeat_response();
            c->send(hb);
        });
        // Setup protocol callbacks
        on_auth([this](auto client, auto message) {
            auto json = message.get_json();
            if (json.at("data")) {
                m_is_authenticated = true;
                logi("authenticated with upstream host");
            } else {
                m_is_authenticated = false;
                this->close();
            }
        })
            .on_implement([this](auto client, auto message) {
                logi("on_implement response received");
            })
            .on_subscribe([this](auto client, auto message) {
                logi("on_subscribe response received");
            })
            .on_task([this](auto client, auto message) {
                logi("on_task request received");
                // IMPORTANT: propagate request to service
                // Propagate request onto a service that implements
                // this event and/or method.
                //
                // In the service class, we'll implement a way to bind
                // handlers to methods or events. method handlers
                // will return a json serializable type, which
                // will fill the "data" key of the response.
                //
                // Example functions:
                //   auto f = [](std::vector<std::string> args) {
                //      return args;
                //   };
                //
                // service.implement("f", f);
                //
                // ...
                // // task for method "f" comes in
                // auto result = service.call("f", std::forward<Args>(args)...);
                // ...
                // auto request_json = message.get_json();
                // response_json["task_id"] = request_json["task_id"];
                //
                // // serializes call result into message data
                // response_json["data"] = result;
                //
                // auto response = response_json.dump();
                // auto header = serialize_header(data_type::task,
                //     action_type::response, response.size());
                // send(data(header, response));
                //
            });

        on_connect([this](auto client) {
            logi("upstream connected (remote = ", client->remote_host(), ":",
                 client->remote_port(), ")");
        })
            .on_read([this](auto client, auto msg) {
                // on_read attempts to call protocol-level handlers
                /*
                try {
                    this->call(msg.type(), client, msg);
                } catch (std::out_of_range &e) {
                    loge("no type handler bound for: ", msg.type());
                } catch (json::parse_error &e) {
                    if (this->has_error()) {
                        this->call_error(client,
                                         boost::asio::error::invalid_argument);
                    }
                } catch (std::exception &e) {
                    loge("exception caught while running call for data type: ",
                         msg.type(), "; message: ", e.what());
                }
                */
                this->call(msg.get_type(), client, msg);
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

private:
    bool m_is_authenticated{false};
    set_log_address;

}; // class upstream

}; // namespace node
}; // namespace ns

#endif // NS_NODE_UPSTREAM_HPP
