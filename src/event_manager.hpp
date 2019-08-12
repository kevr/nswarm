#ifndef NS_EVENT_MANAGER_HPP
#define NS_EVENT_MANAGER_HPP

#include "manager.hpp"
#include <nswarm/task.hpp>

namespace ns
{

template <typename Connection>
class event_manager : public manager<Connection, net::task>
{
public:
    void broadcast(net::task t)
    {
        auto &set = this->m_connections.at(t.json().at("event"));
        for (auto &connection : set) {
            connection->send(t);
        }
    }

    void broadcast(const std::string &task_id, const std::string &event)
    {
        // Make an event task.
        auto t = net::task(task_id);
        t.set_event(event);

        auto &set = this->m_connections.at(event);
        for (auto &connection : set) {
            connection->send(t);
        }
    }
};

}; // namespace ns

#endif
