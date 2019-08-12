#ifndef NS_MANAGER_HPP
#define NS_MANAGER_HPP

#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace ns
{

template <typename Connection, typename Data>
class manager
{
protected:
    using ConnectionPtr = std::shared_ptr<Connection>;
    using ConnectionSet = std::unordered_set<ConnectionPtr>;
    std::unordered_map<std::string, ConnectionSet> m_connections;

public:
    void add(const std::string &key, ConnectionPtr connection)
    {
        m_connections[key].emplace(std::move(connection));
    }

    void remove(const std::string &key, ConnectionPtr connection)
    {
        auto &set = m_connections.at(key);
        auto iter = set.find(connection);
        if (iter != set.end())
            set.erase(iter);
    }

    // Remove a connection from the entire manager.
    void remove(ConnectionPtr connection)
    {
        for (auto &kv : m_connections) {
            auto &set = kv.second;
            auto iter = set.find(connection);
            if (iter != set.end())
                set.erase(iter);
        }
    }
};

}; // namespace ns

#endif
