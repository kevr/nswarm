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

namespace net
{

/**
 * Layout: [16:type][14:args][1:error][1:direction][32:size]
 *
 * We decide to omit storage for error and direction to keep
 * alignment at 64-bits. We bit shift to extract the separate
 * fields from the argument section.
 *
 * This header should be encapsulated inside of net::message
 * below, or an object derived from net::message.
 **/
class header
{
    uint16_t m_type{0};
    uint16_t m_args{0};
    uint32_t m_size{0};

public:
    header() = default;

    // Shift/cast and mask out different fields of the header.
    header(uint64_t bits)
        : m_type(bits >> 48)       // Left-most 16 bits
        , m_args(bits >> 32)       // The next 16 bits
        , m_size((uint32_t)(bits)) // Right-most 32 bits
    {
    }

    header(uint16_t type, uint16_t args, uint8_t error, uint8_t direction,
           uint32_t size)
        : m_type(type)
        // Take the right-most 14 bits in args and the right-most
        // bit in the other two to pack an internal 16-bit args integer
        , m_args((args << 2) | ((error & 1) << 1) | (direction & 1))
        , m_size(size)
    {
    }

    const uint64_t value() const
    {
        return pack(m_type, m_args, m_size);
    }

    const uint16_t type() const
    {
        return m_type;
    }

    const uint16_t args() const
    {
        // Just shift args on top of error/direction
        return m_args >> 2;
    }

    const uint8_t error() const
    {
        return (uint8_t)(m_args >> 1) & 1;
    }

    const uint8_t direction() const
    {
        return (uint8_t)(m_args & 1);
    }

    const uint32_t size() const
    {
        return m_size;
    }

public: // Static functions
    static uint64_t pack(uint16_t type, uint16_t args, uint8_t error,
                         uint8_t direction, uint32_t size)
    {
        // Pack everything into a 64-bit value.
        return (uint64_t(type) << 48) | (uint64_t(args) << 32) |
               (uint64_t(error & 1) << 33) | (uint64_t(direction & 1) << 32) |
               uint64_t(size);
    }

    static uint64_t pack(uint16_t type, uint16_t args, uint32_t size)
    {
        return (uint64_t(type) << 48) | (uint64_t(args) << 32) | uint64_t(size);
    }
};

struct error {
    // net::error::type::*
    enum type : uint8_t { none = 0x0, set = 0x1, bad };

    // Variant tags
    struct tag {
        struct none {
            static constexpr error::type type = error::type::none;
            static constexpr const char *const human = "error::type::none";
        };

        // set means that an error exists.
        struct set {
            static constexpr error::type type = error::type::set;
            static constexpr const char *const human = "error::type::set";
        };

        struct bad {
            static constexpr error::type type = error::type::bad;
            static constexpr const char *const human = "error::type::bad";
        };
    };

    using variant = std::variant<tag::none, tag::set, tag::bad>;

    // Deduce a variant from a real-time error::type value.
    //
    // Example:
    //     ns::match(deduce(msg.error()),
    //         [](error::type::set) {
    //             std::cerr << "Got an error.";
    //         });
    //
    static constexpr variant deduce(error::type t)
    {
        switch (t) {
        case error::type::none:
            return tag::none{};
        case error::type::set:
            return tag::set{};
        case error::type::bad:
            return tag::bad{};
        }
        return tag::bad{};
    }
    static constexpr variant deduce(uint8_t t)
    {
        return deduce(static_cast<error::type>(t));
    }
};

struct action {
    // net::action::type::*
    enum type : uint8_t { request = 0x0, response = 0x1, bad };

    struct tag {
        struct request {
            static constexpr action::type type = action::type::request;
            static constexpr const char *const human = "action::type::request";
        };
        struct response {
            static constexpr action::type type = action::type::response;
            static constexpr const char *const human = "action::type::response";
        };
        struct bad {
            static constexpr action::type type = action::type::bad;
            static constexpr const char *const human = "action::type::bad";
        };
    };

    using variant = std::variant<tag::request, tag::response, tag::bad>;

    // Deduce a variant from a real-time action::type value
    static constexpr variant deduce(action::type t)
    {
        switch (t) {
        case action::type::request:
            return tag::request{};
        case action::type::response:
            return tag::response{};
        case action::type::bad:
            return tag::bad{};
        }
        // Otherwise, return bad
        return tag::bad{};
    }
    static constexpr variant deduce(uint8_t t)
    {
        return deduce(static_cast<action::type>(t));
    }
};

// Namespace this class to avoid conflicts
namespace impl
{

class message
{
protected:
    net::header m_header;
    std::string m_data;

public: // Some constant structure data
    // net::message::type::*
    enum type : uint16_t {
        auth = 0x1,
        implement = 0x2,
        subscribe = 0x3,
        task = 0x4,
        bad,
    };

    // Variant tags
    struct tag {
        struct auth {
            static constexpr message::type type = message::type::auth;
            static constexpr const char *const human = "message::type::auth";
        };
        struct implement {
            static constexpr message::type type = message::type::implement;
            static constexpr const char *const human =
                "message::type::implement";
        };
        struct subscribe {
            static constexpr message::type type = message::type::subscribe;
            static constexpr const char *const human =
                "message::type::subscribe";
        };
        struct task {
            static constexpr message::type type = message::type::task;
            static constexpr const char *const human = "message::type::task";
        };
        struct bad {
            static constexpr message::type type = message::type::bad;
            static constexpr const char *const human = "message::type::bad";
        };
    };

    using variant = std::variant<tag::auth, tag::implement, tag::subscribe,
                                 tag::task, tag::bad>;

    // Deduce a variant from a real-time message::type value
    static constexpr variant deduce(message::type t)
    {
        switch (t) {
        case message::type::auth:
            return tag::auth{};
        case message::type::implement:
            return tag::implement{};
        case message::type::subscribe:
            return tag::subscribe{};
        case message::type::task:
            return tag::task{};
        case message::type::bad:
            return tag::bad{};
        }
        return tag::bad{};
    }
    static constexpr variant deduce(uint16_t t)
    {
        return deduce(static_cast<message::type>(t));
    }

public: // Message constructors
    message() = default;

    message(const message &msg)
        : m_header(msg.m_header)
        , m_data(msg.m_data)
    {
    }

    void operator=(const message &msg)
    {
        m_header = msg.m_header;
        m_data = msg.m_data;
    }

    message(message &&msg)
        : m_header(msg.m_header)
        , m_data(std::move(msg.m_data))
    {
    }

    void operator=(message &&msg)
    {
        m_header = msg.m_header;
        m_data = std::move(msg.m_data);
    }

public: // Initialization constructors
    message(const net::header &head)
        : m_header(head)
    {
    }

    message(const net::header &head, std::string data)
        : m_header(head)
        , m_data(std::move(data))
    {
    }

    // Provide three update overloads for different situations.
    // Messages can be partially correct while we have not yet
    // received payload data.
    void update(const net::header &head)
    {
        m_header = head;
    }

    void update(std::string data)
    {
        m_data = std::move(data);
        // Fix header size field if it's not yet matched
        if (m_header.size() != m_data.size()) {
            m_header =
                net::header(m_header.type(), m_header.args(), m_header.error(),
                            m_header.direction(), m_data.size());
        }
    }

    void update(const net::header &head, std::string data)
    {
        update(head);
        update(std::move(data));
    }

public:
    const net::error::type get_error() const
    {
        return static_cast<net::error::type>(m_header.error());
    }

    const bool has_error() const
    {
        return get_error() == error::type::set;
    }

    const net::action::type get_action() const
    {
        return static_cast<net::action::type>(m_header.direction());
    }

    const message::type get_type() const
    {
        return static_cast<message::type>(m_header.type());
    }

    const net::header &head() const

    {
        return m_header;
    }

    const std::string &data() const
    {
        return m_data;
    }

    const std::string &get_string() const
    {
        return data();
    }
};
}; // namespace impl

using message = impl::message;

class json_message : public net::message
{
protected:
    ns::json m_json;

public:
    // Inherit constructors
    using message::message;
    using message::update;

    // Some overloads for json updates
    json_message(const net::header &head, ns::json data)
        : message(head)
    {
        update(std::move(data));
    }

    json_message(const json_message &msg)
        : message(msg) // Delegate header over to base
    {
        m_json = msg.m_json;
    }

    void operator=(const json_message &msg)
    {
        message::operator=(msg);
        m_json = msg.m_json;
    }

    json_message(json_message &&msg)
        : message(std::move(msg)) // Delegate header over to base
    {
        m_json = std::move(msg.m_json);
    }

    void operator=(json_message &&msg)
    {
        message::operator=(std::move(msg));
        m_json = std::move(msg.m_json);
    }

    void update(ns::json data)
    {
        m_json = std::move(data);
        update(m_json.dump());
    }

    const ns::json &json() const
    {
        return m_json;
    }

    const ns::json &get_json()
    {
        if (!m_json.size())
            m_json = json::parse(m_data);
        return json();
    }
};

}; // namespace net

using json_message = net::json_message;

namespace value
{
// Start this value at 1, we don't want to accept 0 as a valid type.
enum data_value : uint16_t {
    auth = 1,  // Authentication: node -> cluster, api -> cluster
    implement, // Provide a method: node -> host
    subscribe, // Subscribe to an event: node -> host
    task,      // Method or event task: api -> host -> node -> host -> api
};
}; // namespace value

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
enum action_value : uint8_t { request = 0, response = 1 };
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

inline uint16_t make_flags(uint16_t params, uint16_t action,
                           bool error = false) noexcept
{
    return (params << 2) | (error ? (1 << 1) : 0) | (action & 1);
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
                                 uint32_t c, bool error = false) noexcept
{
    logd("serialize_header(", a, ", ", ba, ", ", bb, ", ", c, ")");
    return serialize_header(a, make_flags(ba, bb, error), c);
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
        return m_flags >> 2;
    }

    // error field
    const bool error() const noexcept
    {
        // Shift over the error bit and mask it
        return (m_flags >> 1) & 1;
    }

    const value::action_value action() const noexcept
    {
        // Mask the action bit
        return static_cast<value::action_value>(m_flags & 1);
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

// "New enum" overload helpers
inline bool operator==(ns::net::error::type lhs, uint8_t rhs)
{
    return lhs == static_cast<ns::net::error::type>(rhs);
}

inline bool operator==(uint8_t lhs, ns::net::error::type rhs)
{
    return static_cast<ns::net::error::type>(lhs) == rhs;
}

inline bool operator==(ns::net::action::type lhs, uint8_t rhs)
{
    return lhs == static_cast<ns::net::action::type>(rhs);
}

inline bool operator==(uint8_t lhs, ns::net::action::type rhs)
{
    return static_cast<ns::net::action::type>(lhs) == rhs;
}

inline bool operator==(ns::net::message::type lhs, uint16_t rhs)
{
    return lhs == static_cast<ns::net::message::type>(rhs);
}

inline bool operator==(uint16_t lhs, ns::net::message::type rhs)
{
    return static_cast<ns::net::message::type>(lhs) == rhs;
}

namespace ns
{

inline net::error::type to_error(uint8_t e)
{
    return static_cast<net::error::type>(e);
}

inline net::action::type to_action(uint8_t e)
{
    return static_cast<net::action::type>(e);
}

inline net::message::type to_type(uint16_t e)
{
    return static_cast<net::message::type>(e);
}

}; // namespace ns

#endif
