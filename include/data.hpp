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

    data(uint16_t type, uint16_t flags,
         std::string data_value = std::string()) noexcept
        : m_type(type)
        , m_flags(flags)
        , m_data(std::move(data_value))
    {
    }

    data(uint64_t packet, std::string data_value = std::string()) noexcept
        : m_data(std::move(data_value))
    {
        m_type = (uint16_t)(packet >> 48);
        m_flags = (uint16_t)(packet >> 32);
        m_size = 0;
        logd("data created with {", m_type, ", ", m_flags, ", ", m_data.size(),
             "}");
    }

    data(const data &other) noexcept
        : m_type(other.m_type)
        , m_flags(other.m_flags)
        , m_data(other.m_data)
    {
    }

    data &operator=(const data &other) noexcept
    {
        m_type = other.m_type;
        m_flags = other.m_flags;
        m_data = other.m_data;
        return *this;
    }

    data(data &&other) noexcept
        : m_type(other.m_type)
        , m_flags(other.m_flags)
        , m_data(std::move(other.m_data))
    {
    }

    data &operator=(data &&other) noexcept
    {
        m_type = other.m_type;
        m_flags = other.m_flags;
        m_data = std::move(other.m_data);
        return *this;
    }

    void read_packet(std::istream &is)
    {
        uint64_t packet = 0;
        is.read(reinterpret_cast<char *>(&packet), sizeof(uint64_t));
        auto [type, flags, size] = deserialize_packet(packet);
        m_type = type;
        m_flags = flags;
        m_size = size;
        *this = data(packet);
    }

    void read_data(std::istream &is)
    {
        std::string value(m_size, '\0');
        is.read(&value[0], m_size);
        m_data = std::move(value);
    }

    const uint64_t packet() const noexcept
    {
        return serialize_packet(m_type, m_flags, m_size);
    }

    // These three bitmask functions mask against the total
    // of their type, with their bits positioned at the
    // lower end of the result from the bitwise shift.
    const uint16_t type() const noexcept
    {
        return m_type;
    }

    const uint16_t flags() const noexcept
    {
        return m_flags;
    }

    const uint32_t size() const noexcept
    {
        return m_size;
    }

    const std::string &get_string() const noexcept
    {
        return m_data;
    }

private:
    uint16_t m_type = 0;
    uint16_t m_flags = 0;
    uint32_t m_size = 0;
    std::string m_data;
};

}; // namespace ns

#endif
