#ifndef NS_IMPLEMENT_HPP
#define NS_IMPLEMENT_HPP

#include <nswarm/data.hpp>

namespace ns
{

// A structure that represents a function implementation
// by a node application.
template <typename A>
class implement : public data_object<implement<A>>
{
    std::string m_method;

    using data_object_t = data_object<implement<A>>;

public:
    using data_object_t::data_object;

    // Allow construction with just a method name.
    // This can be used by the requester to make a fresh
    // implement data object.
    implement(const std::string &method)
        : data_object_t(
              serialize_header(data_type::implement::value, 0, A::value, 0))
    {
        this->m_json["method"] = method;
        this->m_data = this->m_json.dump();
        this->m_method = method;
        this->set_size(this->get_string().size());
    }

    // Provide all possible combinations of implement<A>
    template <typename U>
    implement(const implement<U> &other)
        : data_object_t(other)
    {
        prepare();
    }

    template <typename U>
    implement &operator=(const implement<U> &other)
    {
        data_object_t::operator=(other);
        return *this;
    }

    template <typename U>
    implement(implement<U> &&other)
        : data_object_t(std::move(other))
    {
        prepare();
    }

    template <typename U>
    implement &operator=(implement<U> &&other)
    {
        data_object_t::operator=(std::move(other));
        return *this;
    }

    const std::string &method() const
    {
        return m_method;
    }

    void prepare()
    {
        m_method = this->get_json().at("method");
    }
};

using implement_request = implement<action_type::request>;
using implement_response = implement<action_type::response>;

}; // namespace ns

#endif // NS_IMPLEMENT_HPP

