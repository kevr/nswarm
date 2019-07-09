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

#include "auth.hpp"
#include "client.hpp"
#include "data.hpp"
#include "protocol.hpp"

namespace ns
{
namespace node
{

class upstream : public client<upstream>, protected protocol<upstream, ns::data>
{
public:
    // We require upstream to be bound to the service server's io
    // so that this connection interleaves with the main executor
    // in the daemon. Creating a separate io_service thread
    // would be overkill.
    upstream(io_service &io)
        : client(io)
        , m_auth("abcd")
    {
        init();
    }

    void auth(const std::string &key)
    {
        json js;
        js["key"] = key;

        auto json_str = js.dump();
        auto header = serialize_header(data_type::auth, action_type::request,
                                       json_str.size());

        auto data = ns::data(header, json_str);
        send(data);
    }

    // construct provide message
    void provide(const std::string &method)
    {
        json js;
        js["method"] = method;

        auto json_str = js.dump();
        auto header = serialize_header(data_type::provide, action_type::request,
                                       json_str.size());

        auto data = ns::data(header, json_str);
        send(data);
    }

    void subscribe(const std::string &event)
    {
        json js;
        js["event"] = event;

        auto json_str = js.dump();
        auto header = serialize_header(data_type::subscribe,
                                       action_type::request, json_str.size());

        auto data = ns::data(header, json_str);
        send(data);
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
    void respond(const std::string &task_id, const T &response)
    {
        json js;
        js["task_id"] = task_id;
        js["data"] = response;

        auto json_str = js.dump();
        auto header = serialize_header(data_type::task, action_type::response,
                                       json_str.size());

        auto data = ns::data(header, json_str);
        send(data);
    }

    virtual ~upstream() = default;

    const bool authenticated() const
    {
        return m_auth.authenticated();
    }

private:
    void init()
    {
        // Setup protocol callbacks
        on_auth([this](auto client, auto message) {
            logi("on_auth response received: ", message.get_string());
            auto json = message.get_json();
            if (json["data"]) {
                m_auth.authenticate(json["key"]);
            }
        })
            .on_provide([this](auto client, auto message) {
                logi("on_provide response received");
            })
            .on_subscribe([this](auto client, auto message) {
                logi("on_subscribe response received");
            })
            .on_task([this](auto client, auto message) {
                logi("on_task request received");
                // IMPORTANT: propagate request to service
                // Propagate request onto a service that provides
                // this event and/or method.
                //
                // In the service class, we'll provide a way to bind
                // handlers to methods or events. method handlers
                // will return a json serializable type, which
                // will fill the "data" key of the response.
                //
                // Example functions:
                //   auto f = [](std::vector<std::string> args) {
                //      return args;
                //   };
                //
                // service.provide("f", f);
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
                try {
                    this->call(msg.type(), client, msg);
                } catch (std::out_of_range &e) {
                    loge("no type handler bound for: ", msg.type());
                } catch (std::exception &e) {
                    loge("exception caught while running call for data type: ",
                         msg.type(), "; message: ", e.what());
                }
            })
            .on_close([this](auto client) {
                // Automatically try to reconnect
                logd("upstream connection closed, reconnecting in 10 seconds");
                std::this_thread::sleep_for(std::chrono::seconds(10));
                client->run(m_host, m_port);
            })
            .on_error([this](auto client, const auto &ec) {
                logd("socket error: ", ec.message());
                logd("upstream connection closed, reconnecting in 10 seconds");
                std::this_thread::sleep_for(std::chrono::seconds(10));
                client->run(m_host, m_port);
            });
        ;
    }

private:
    auth_context<authentication::plain> m_auth;
    set_log_address;

}; // class upstream

}; // namespace node
}; // namespace ns

#endif // NS_NODE_UPSTREAM_HPP
