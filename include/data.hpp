#ifndef NS_DATA_HPP
#define NS_DATA_HPP

#include "json.hpp"
#include "logging.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <tuple>

namespace ns
{

// Alias json here, thanks nlohmann! <3
using nlohmann::json;

enum data_type : uint16_t {
    auth = 1,  // Authentication: node -> cluster, api -> cluster
    provide,   // Provide a method: node -> host
    subscribe, // Subscribe to an event: node -> host
    task,      // Method or event task: api -> host -> node -> host -> api

};

inline std::tuple<uint16_t, uint16_t, uint32_t>
deserialize_packet(uint64_t data)
{
    uint16_t a = (uint16_t)(data >> 48);
    uint16_t b = (uint16_t)(data >> 32);
    uint32_t c = (uint32_t)data;
    return std::make_tuple(a, b, c);
}

// [16 bytes message_type][16 bytes arbitrary_flags][32 bytes data_size]
inline uint64_t serialize_packet(uint16_t a, uint16_t b, uint32_t c)
{
    logd("serialize_packet(", a, ", ", b, ", ", c, ")");
    uint64_t data = 0;
    data |= (uint64_t)a << 48;
    data |= (uint64_t)b << 32;
    data |= (uint64_t)c;

    auto [ax, bx, cx] = deserialize_packet(data);

    logd("deserialize_packet(", ax, ", ", bx, ", ", cx, ")");

    return data;
}

class data
{
public:
    data(uint64_t pkt, std::string data_value = std::string())
        : m_packet(pkt)
    {
        if (data_value.size() > 0)
            m_data = std::move(data_value);
    }

    data(uint16_t type, uint16_t flags, std::string data_value = std::string())
    {
        // Here: if data is present, serialize_packet with the real data size
        // and set data_size at the same time.
        m_packet = serialize_packet(type, flags, data_value.size());
        if (data_value.size() > 0)
            m_data = std::move(data_value);
    }

    const uint64_t packet() const
    {
        return m_packet;
    }

    // These three bitmask functions mask against the total
    // of their type, with their bits positioned at the
    // lower end of the result from the bitwise shift.
    const uint16_t type() const
    {
        return (uint16_t)(m_packet >> 48);
    }

    const uint16_t flags() const
    {
        return (uint16_t)(m_packet >> 32);
    }

    const uint32_t size() const
    {
        return (uint32_t)m_packet;
    }

    const json &get_json()
    {
        if (!m_json.size()) {
            if (!m_data.size()) {
                throw std::out_of_range("no data present to parse as json");
            }
            m_json = json::parse(m_data);
        }
        return m_json;
    }

    const std::string &get_string() const
    {
        return m_data;
    }

private:
    uint64_t m_packet;

    // m_data should always be valid json.
    // get_json() will throw if m_json and/or m_data is empty.
    std::string m_data;

    json m_json;
};

}; // namespace ns

#endif
