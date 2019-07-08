#ifndef NS_DATA_HPP
#define NS_DATA_HPP

#include "json.hpp"
#include "logging.hpp"
#include <bitset>
#include <cassert>
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

enum action_type : uint16_t { request, response };

inline std::tuple<uint16_t, uint16_t, uint32_t>
deserialize_packet(uint64_t data) noexcept
{
    logd("deserialize_packet = ", std::bitset<64>(data));
    uint16_t a = (uint16_t)(data >> 48);
    uint16_t b = (uint16_t)(data >> 32);
    uint32_t c = (uint32_t)data;
    logd("deserialize_packet(", a, ", ", b, ", ", c, ")");
    return std::make_tuple(a, b, c);
}

// [16 bytes message_type][16 bytes arbitrary_flags][32 bytes data_size]
inline uint64_t serialize_packet(uint16_t a, uint16_t b, uint32_t c) noexcept
{
    logd("serialize_packet(", a, ", ", b, ", ", c, ")");
    uint64_t data = 0;
    data |= ((uint64_t)(a)) << 48;
    data |= ((uint64_t)(b)) << 32;
    data |= ((uint64_t)(c));
    logd("serialize_packet = ", std::bitset<64>(data));
    return data;
}

class data
{
public:
    data() = default;

    data(uint64_t packet, std::string data_value = std::string()) noexcept
        : m_data(std::move(data_value))
    {
        m_type = (uint16_t)(packet >> 48);
        m_flags = (uint16_t)(packet >> 32);
        m_size = (uint32_t)packet;
        logd("data created with {", m_type, ", ", m_flags, ", ", m_size,
             "}, real data size = ", m_data.size());
    }

    data(const data &other) noexcept
        : m_type(other.m_type)
        , m_flags(other.m_flags)
        , m_size(other.m_size)
        , m_data(other.m_data)
    {
    }

    data &operator=(const data &other) noexcept
    {
        m_type = other.m_type;
        m_flags = other.m_flags;
        m_size = other.m_size;
        m_data = other.m_data;
        return *this;
    }

    data(data &&other) noexcept
        : m_type(other.m_type)
        , m_flags(other.m_flags)
        , m_size(other.m_size)
        , m_data(std::move(other.m_data))
    {
    }

    data &operator=(data &&other) noexcept
    {
        m_type = other.m_type;
        m_flags = other.m_flags;
        m_size = other.m_size;
        m_data = std::move(other.m_data);
        return *this;
    }

    void read_packet(std::istream &is)
    {
        trace();
        uint64_t packet = 0;
        is.read(reinterpret_cast<char *>(&packet), sizeof(uint64_t));
        *this = data(packet);
        logd("data updated with packet data = { ", m_type, ", ", m_flags, ", ",
             m_size, " }");
    }

    void read_data(std::istream &is)
    {
        trace();
        std::string value(m_size, '\0');
        is.read(&value[0], m_size);
        m_data = std::move(value);
        logd("data updated with real data size = ", m_data.size());
    }

    const uint64_t packet() const noexcept
    {
        return serialize_packet(m_type, m_flags, m_size);
    }

    // These three bitmask functions mask against the total
    // of their type, with their bits positioned at the
    // lower end of the result from the bitwise shift.
    const ns::data_type type() const noexcept
    {
        return static_cast<ns::data_type>(m_type);
    }

    const ns::action_type flags() const noexcept
    {
        return static_cast<ns::action_type>(m_flags);
    }

    const uint32_t size() const noexcept
    {
        return m_size;
    }

    const std::string &get_string() const noexcept
    {
        return m_data;
    }

    const json &get_json()
    {
        if (!m_json.size())
            m_json = json::parse(m_data);
        return m_json;
    }

private:
    uint16_t m_type = 0;
    uint16_t m_flags = 0;
    uint32_t m_size = 0;
    std::string m_data;

    json m_json;

protected:
    set_log_address;
};

}; // namespace ns

#endif
