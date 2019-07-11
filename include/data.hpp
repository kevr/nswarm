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

enum action_type : uint16_t { request = 0, response = 1 };

inline uint16_t make_flags(uint16_t params, uint16_t action) noexcept
{
    return (params << 1) | (action & 1);
}

inline std::tuple<uint16_t, uint16_t, uint32_t>
deserialize_header(uint64_t data) noexcept
{
    logd("deserialize_header = ", std::bitset<64>(data));
    uint16_t a = (uint16_t)(data >> 48);
    uint16_t b = (uint16_t)(data >> 32);
    uint32_t c = (uint32_t)data;
    logd("deserialize_header(", a, ", ", b, ", ", c, ")");
    return std::make_tuple(a, b, c);
}

// [16 bytes message_type][16 bytes arbitrary_flags][32 bytes data_size]
inline uint64_t serialize_header(uint16_t a, uint16_t b, uint32_t c) noexcept
{
    logd("serialize_header(", a, ", ", b, ", ", c, ")");
    uint64_t data = 0;
    data |= ((uint64_t)(a)) << 48;
    data |= ((uint64_t)(b)) << 32;
    data |= ((uint64_t)(c));
    logd("serialize_header = ", std::bitset<64>(data));
    return data;
}

// [16 bits message_type][15 bits params][1 bit direction][32 bits data_size]
inline uint64_t serialize_header(uint16_t a, uint16_t ba, uint16_t bb,
                                 uint32_t c) noexcept
{
    logd("serialize_header(", a, ", ", ba, ", ", bb, ", ", c, ")");
    return serialize_header(a, make_flags(ba, bb), c);
}

class data
{
public:
    data() = default;

    data(uint64_t header, std::string data_value = std::string()) noexcept
        : m_data(std::move(data_value))
    {
        m_type = (uint16_t)(header >> 48);
        m_flags = (uint16_t)(header >> 32);
        m_size = (uint32_t)header;
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

    void read_header(std::istream &is)
    {
        uint64_t header = 0;
        is.read(reinterpret_cast<char *>(&header), sizeof(uint64_t));
        *this = data(header);
        logd("data updated with header data = { ", m_type, ", ", m_flags, ", ",
             m_size, " }");
    }

    void read_data(std::istream &is)
    {
        std::string value(m_size, '\0');
        is.read(&value[0], m_size);
        m_data = std::move(value);
        logd("data updated with real data size = ", m_data.size());
    }

    const uint64_t header() const noexcept
    {
        return serialize_header(m_type, m_flags, m_size);
    }

    // These three bitmask functions mask against the total
    // of their type, with their bits positioned at the
    // lower end of the result from the bitwise shift.
    const ns::data_type type() const noexcept
    {
        return static_cast<ns::data_type>(m_type);
    }

    // Actually 24 bytes of data
    const uint16_t params() const noexcept
    {
        // the left-most 15 bits
        return m_flags >> 1;
    }

    const ns::action_type direction() const noexcept
    {
        // the right-most bit
        const uint16_t DIRECTION_MASK = 1;
        return static_cast<ns::action_type>(m_flags & DIRECTION_MASK);
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

inline std::string data_type_string(const data_type &type)
{
    static const std::map<data_type, std::string> types{
        {data_type::auth, "data_type::auth"},
        {data_type::provide, "data_type::provide"},
        {data_type::subscribe, "data_type::subscribe"},
        {data_type::task, "data_type::task"},
    };
    return types.at(type);
}

inline std::string action_type_string(const action_type &type)
{
    static const std::map<action_type, std::string> types{
        {action_type::request, "action_type::request"},
        {action_type::response, "action_type::response"},
    };
    return types.at(type);
}

}; // namespace ns

inline std::stringstream &operator<<(std::stringstream &os,
                                     const ns::data_type &type)
{
    os << ns::data_type_string(type);
    return os;
}

inline std::stringstream &operator<<(std::stringstream &os,
                                     const ns::action_type &type)
{
    os << ns::action_type_string(type);
    return os;
}

#endif
