#ifndef NS_DATA_HPP
#define NS_DATA_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <tuple>

namespace ns
{

enum message_type : uint16_t {
    auth = 1,  // Auth: node -> cluster, api -> cluster
    provide,   // Provide: node -> host
    subscribe, // Subscribe: node -> host
    task,      // Task: api -> host -> node -> host -> api

};

inline uint64_t serialize_packet(uint16_t a, uint16_t b, uint32_t c)
{
    uint64_t data = 0;
    data |= a;
    data <<= 16;
    data |= b;
    data <<= 32;
    data |= c;
    return data;
}

inline std::tuple<uint16_t, uint16_t, uint32_t>
deserialize_packet(uint64_t data)
{
    uint16_t a = (data >> 48) & 0xFF;
    uint16_t b = (data >> 32) & 0xFF;
    uint32_t c = data & 0xFFFF;
    return std::make_tuple(a, b, c);
}

struct message {
public:
    message(uint64_t pkt, std::optional<std::string> data)
        : m_pkt(pkt)
    {
        if (data)
            m_data = *data;
    }

    const std::string &data() const
    {
        return m_data;
    }

    const uint32_t data_size() const
    {
        const uint32_t mask = 0xFFFF;
        return mask & m_pkt;
    }

    const uint16_t type() const
    {
        return m_pkt >> 48;
    }

    const uint16_t flags() const
    {
        return m_pkt >> 32;
    }

private:
    // pkt: [16 bits flags][16 bits flags][32 bits data size]
    uint64_t m_pkt;
    std::string m_data;
};

}; // namespace ns

#endif
