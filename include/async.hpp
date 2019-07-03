/**
 * \project nswarm
 * \file async.hpp
 * \description Asynchronous helper objects and classes.
 **/
#ifndef NS_ASYNC_HPP
#define NS_ASYNC_HPP

#include "data.hpp"
#include <functional>
#include <memory>
#include <string>

namespace ns {

template <typename T>
using async_read_function =
    std::function<void(std::shared_ptr<T>, const message &)>;

template <typename T>
using async_connect_function = std::function<void(std::shared_ptr<T>)>;

template <typename T>
using async_close_function = std::function<void(std::shared_ptr<T>)>;

template <typename T>
using async_error_function =
    std::function<void(std::shared_ptr<T>, std::exception)>;

template <typename T>
class async_object
{
  public:
    async_object &on_read(async_read_function<T> f)
    {
        m_read_f = f;
    }

    async_object &on_connect(async_connect_function<T> f)
    {
        m_connect_f = f;
    }

    async_object &on_close(async_close_function<T> f)
    {
        m_close_f = f;
    }

    async_object &on_error(async_error_function<T> f)
    {
        m_error_f = f;
    }

  protected:
    async_read_function<T> m_read_f;
    async_connect_function<T> m_connect_f;
    async_close_function<T> m_close_f;
    async_error_function<T> m_error_f;
};

}; // namespace ns

#endif