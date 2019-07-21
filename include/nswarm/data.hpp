#ifndef NS_DATA_HPP
#define NS_DATA_HPP

#include <bitset>
#include <cassert>
#include <cstdint>
#include <nswarm/json.hpp>
#include <nswarm/logging.hpp>
#include <nswarm/variant.hpp>
#include <optional>
#include <string>
#include <tuple>

namespace ns
{

// Alias json here, thanks nlohmann! <3
using nlohmann::json;

namespace value
{
enum data_value : uint16_t {
    auth = 1,  // Authentication: node -> cluster, api -> cluster
    implement, // Provide a method: node -> host
    subscribe, // Subscribe to an event: node -> host
    task,      // Method or event task: api -> host -> node -> host -> api
};
};

namespace data_type
{

struct auth {
    static constexpr value::data_value value = value::data_value::auth;
};
struct implement {
    static constexpr value::data_value value = value::data_value::implement;
};
struct subscribe {
    static constexpr value::data_value value = value::data_value::subscribe;
};
struct task {
    static constexpr value::data_value value = value::data_value::task;
};

using variant = std::variant<auth, implement, subscribe, task>;

// Required conversion function from actual data_type numeric value to variant
inline variant deduce(const value::data_value t)
{
    switch (t) {
    case value::data_value::auth:
        return auth();
    case value::data_value::implement:
        return implement();
    case value::data_value::subscribe:
        return subscribe();
    case value::data_value::task:
        return task();
    }
    throw std::invalid_argument("Unknown action type: " + std::to_string(t));
}

}; // namespace data_type

namespace value
{
enum action_value : uint16_t { request = 0, response = 1 };
};

namespace action_type
{

struct request {
    static constexpr value::action_value value = value::action_value::request;
};
struct response {
    static constexpr value::action_value value = value::action_value::response;
};

using variant = std::variant<request, response>;

// Required conversion function from actual action_value numeric value to
// variant
inline variant deduce(const value::action_value t)
{
    switch (t) {
    case value::action_value::request:
        return request();
    case value::action_value::response:
        return response();
    }
    throw std::invalid_argument("Unknown action type: " + std::to_string(t));
}

}; // namespace action_type

inline uint16_t make_flags(uint16_t params, uint16_t action) noexcept
{
    return (params << 1) | (action & 1);
}

inline std::tuple<uint16_t, uint16_t, uint32_t>
deserialize_header(uint64_t data) noexcept
{
    uint16_t a = (uint16_t)(data >> 48);
    uint16_t b = (uint16_t)(data >> 32);
    uint32_t c = (uint32_t)data;
    logd("deserialize_header(", a, ", ", b, ", ", c, ")");
    logd("deserialize_header = ", std::bitset<64>(data));
    return std::make_tuple(a, b, c);
}

// [16 bytes message_type][16 bytes arbitrary_flags][32 bytes data_size]
inline uint64_t serialize_header(uint16_t a, uint16_t b, uint32_t c) noexcept
{
    uint64_t data = 0;
    data |= ((uint64_t)(a)) << 48;
    data |= ((uint64_t)(b)) << 32;
    data |= ((uint64_t)(c));
    logd("serialize_header(", a, ", ", b, ", ", c, ")");
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

// T: Derivative class
// T is used to represent a higher level of data, for example, authentication.
// T should implement the following public functions for initialization:
//     void prepare()
//
template <typename T>
class basic_data
{
public:
    basic_data() = default;

    basic_data(uint64_t header, const json &js) noexcept
        : basic_data(header, js.dump())
    {
        tracevalue();
        logd("basic_data created with: ", std::bitset<64>(this->header()),
             ", data: ", this->get_string());
    }

    basic_data(uint64_t header, std::string data_value = std::string()) noexcept
        : m_data(std::move(data_value))
    {
        tracevalue();
        m_type = (uint16_t)(header >> 48);
        m_flags = (uint16_t)(header >> 32);
        m_size = (uint32_t)header;
        logd("basic_data created with {", m_type, ", ", m_flags, ", ", m_size,
             "}, real basic_data size = ", m_data.size());
    }

    basic_data(const basic_data &other) noexcept
        : m_type(other.m_type)
        , m_flags(other.m_flags)
        , m_size(other.m_size)
        , m_data(other.m_data)
        , m_json(other.m_json)
    {
        tracecopy();
    }

    template <typename U>
    basic_data(const basic_data<U> &other) noexcept
        : m_type(other.m_type)
        , m_flags(other.m_flags)
        , m_size(other.m_size)
        , m_data(other.m_data)
        , m_json(other.m_json)
    {
        tracecopy();
    }

    T &operator=(const basic_data &other) noexcept
    {
        tracecopy();
        m_type = other.m_type;
        m_flags = other.m_flags;
        m_size = other.m_size;
        m_data = other.m_data;
        m_json = other.m_json;
        auto *ptr = static_cast<T *>(this);
        ptr->prepare();
        return *ptr;
    }

    basic_data(basic_data &&other) noexcept
        : m_type(other.m_type)
        , m_flags(other.m_flags)
        , m_size(other.m_size)
        , m_data(std::move(other.m_data))
        , m_json(std::move(other.m_json))
    {
        tracemove();
    }

    template <typename U>
    basic_data(basic_data<U> &&other) noexcept
        : m_type(other.m_type)
        , m_flags(other.m_flags)
        , m_size(other.m_size)
        , m_data(std::move(other.m_data))
        , m_json(std::move(other.m_json))
    {
        tracemove();
    }

    T &operator=(basic_data &&other) noexcept
    {
        tracemove();
        m_type = other.m_type;
        m_flags = other.m_flags;
        m_size = other.m_size;
        m_data = std::move(other.m_data);
        m_json = std::move(other.m_json);
        auto *ptr = static_cast<T *>(this);
        ptr->prepare();
        return *ptr;
    }

    void read_header(std::istream &is)
    {
        uint64_t header = 0;
        is.read(reinterpret_cast<char *>(&header), sizeof(uint64_t));
        std::tie(m_type, m_flags, m_size) = deserialize_header(header);
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
    const value::data_value type() const noexcept
    {
        return static_cast<value::data_value>(m_type);
    }

    // Actually 24 bytes of data
    const uint16_t params() const noexcept
    {
        // the left-most 15 bits
        return m_flags >> 1;
    }

    const value::action_value direction() const noexcept
    {
        // the right-most bit
        const uint16_t DIRECTION_MASK = 1;
        return static_cast<value::action_value>(m_flags & DIRECTION_MASK);
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

    const json &get_json()
    {
        if (!m_json.size())
            m_json = json::parse(m_data);
        return m_json;
    }

protected:
    void set_size(uint32_t size)
    {
        trace();
        m_size = size;
    }

protected:
    uint16_t m_type = 0;
    uint16_t m_flags = 0;
    uint32_t m_size = 0;
    std::string m_data;

    json m_json;

    template <typename U>
    friend class basic_data;
};

// L0 data: This L0 data is protocol unaware
class data : public basic_data<data>
{
public:
    using basic_data::basic_data;

    void prepare()
    {
        // Do nothing here; noop
    }

protected:
    // data_object<T> shall be a class that can manipulate
    // moved from objects
    template <typename T>
    friend class data_object;
};

// L1 data: This L1 data is data type aware
template <typename T>
class data_object : public basic_data<T>
{
public:
    using basic_data<T>::basic_data;
};

// L2 data: L2 data shall be derivatives of data_object
//          L2 objects are protocol aware

// Here, make and prepare a data_object:
// void DataType::prepare() is required to be implemented.
template <typename DataType, typename... Args>
DataType make_data(Args &&... args)
{
    auto data = DataType(std::forward<Args>(args)...);
    data.prepare();
    return data;
}

inline std::string data_value_string(const value::data_value type)
{
    static const std::map<value::data_value, std::string> types{
        {value::data_value::auth, "data_value::auth"},
        {value::data_value::implement, "data_value::implement"},
        {value::data_value::subscribe, "data_value::subscribe"},
        {value::data_value::task, "data_value::task"},
    };
    return types.at(type);
}

inline std::string action_value_string(const value::action_value type)
{
    static const std::map<value::action_value, std::string> types{
        {value::action_value::request, "action_value::request"},
        {value::action_value::response, "action_value::response"},
    };
    return types.at(type);
}

}; // namespace ns

inline std::stringstream &operator<<(std::stringstream &os,
                                     const ns::value::data_value type)
{
    os << ns::data_value_string(type);
    return os;
}

inline std::stringstream &operator<<(std::stringstream &os,
                                     const ns::value::action_value type)
{
    os << ns::action_value_string(type);
    return os;
}

#endif
