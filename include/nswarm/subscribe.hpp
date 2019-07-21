#ifndef NS_SUBSCRIBE_HPP
#define NS_SUBSCRIBE_HPP

#include <nswarm/data.hpp>

namespace ns
{

// A structure that represents a function subscribeation
// by a node application.
template <typename A>
class subscribe : public data_object<subscribe<A>>
{
    std::string m_event;

    using data_object_t = data_object<subscribe<A>>;

public:
    using data_object_t::data_object;

    // Allow construction with just a event name.
    // This can be used by the requester to make a fresh
    // subscribe data object.
    subscribe(const std::string &event)
        : m_event(event)
    {
        this->m_json["event"] = event;
    }

    // Provide all possible combinations of subscribe<A>
    subscribe(const subscribe &other)
        : data_object_t(other)
    {
        prepare();
    }

    template <typename U>
    subscribe(const subscribe<U> &other)
        : data_object_t(other)
    {
        prepare();
    }

    subscribe &operator=(const subscribe &other)
    {
        data_object_t::operator=(other);
        return *this;
    }

    template <typename U>
    subscribe &operator=(const subscribe<U> &other)
    {
        data_object_t::operator=(other);
        return *this;
    }

    subscribe(subscribe &&other)
        : data_object_t(std::move(other))
    {
        prepare();
    }

    template <typename U>
    subscribe(subscribe<U> &&other)
        : data_object_t(std::move(other))
    {
        prepare();
    }

    subscribe &operator=(subscribe &&other)
    {
        data_object_t::operator=(std::move(other));
        return *this;
    }

    template <typename U>
    subscribe &operator=(subscribe<U> &&other)
    {
        data_object_t::operator=(std::move(other));
        return *this;
    }

    const std::string &event() const
    {
        return m_event;
    }

    void prepare()
    {
        m_event = this->get_json().at("event");
    }
};

using subscribe_request = subscribe<action_type::request>;
using subscribe_response = subscribe<action_type::response>;

}; // namespace ns

#endif // NS_SUBSCRIBE_HPP
