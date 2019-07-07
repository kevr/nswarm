/**
 * \project nswarm
 * \file config.hpp
 *
 * \description Program configuration utility classes and functions.
 *
 * Copyright (c) 2019 Kevin Morris
 * All Rights Reserved.
 **/
#ifndef NS_CONFIG_HPP
#define NS_CONFIG_HPP

// c++ standard library
#include <boost/program_options.hpp>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

// posix standard library
#include <libgen.h>

namespace ns
{
/**
 * \class program_options
 * \brief Deduces program name and arguments.
 **/
class program_options
{
public:
    program_options(const std::string &desc)
        : m_desc(desc)
    {
        init();
    }

    program_options(int argc, const char *argv[], const std::string &desc)
        : m_desc(desc)
    {
        init();
        parse(argc, argv);
    }

    // m_desc is non-copyable. this thing isn't copyable/movable
    program_options(const program_options &) = delete;
    void operator=(const program_options &) = delete;
    program_options(program_options &&) = delete;
    void operator=(program_options &&) = delete;

    // Parse arguments in here. The rest of this object should
    // just provided metadata about the program arguments.
    void parse(int argc, const char *argv[])
    {
        m_executable = basename(const_cast<char *>(argv[0]));
        try {
            boost::program_options::store(
                boost::program_options::parse_command_line(argc, argv, m_desc),
                m_vm);
            boost::program_options::notify(m_vm);
            m_valid = true;
        } catch (std::exception &e) {
            std::cout << "error: " << e.what() << std::endl << std::endl;
            m_valid = false;
        }
    }

    template <typename T>
    program_options &add_option(const std::string &name,
                                const std::string &help)
    {
        m_desc.add_options()(name.c_str(), boost::program_options::value<T>(),
                             help.c_str());
        return *this;
    }

    program_options &add_option(const std::string &name,
                                const std::string &help)
    {
        m_desc.add_options()(name.c_str(), help.c_str());
        return *this;
    }

    bool exists(const std::string &key)
    {
        return m_vm.count(key);
    }

    template <typename T>
    T get(const std::string &key) const
    {
        return m_vm.at(key).as<T>();
    }

    const std::string &name() const
    {
        return m_executable;
    }

    const std::string usage() const
    {
        return "usage: " + m_executable + " [-h] [-d]";
    }

    operator bool() const
    {
        return m_valid;
    }

    int help()
    {
        std::cout << usage() << std::endl << std::endl << *this << std::endl;
        return 1;
    }

private:
    void init()
    {
        add_option("help,h", "Print help message");
        add_option("debug,d", "Enable debug logging");
        add_option("trace", "Enable trace logging (also enables debug)");
        add_option<std::string>("log", "Path to optional logfile");
    }

private:
    bool m_valid = true;
    std::string m_executable;

    boost::program_options::variables_map m_vm;
    boost::program_options::options_description m_desc;

    friend std::ostream &operator<<(std::ostream &os,
                                    const ns::program_options &opt)
    {
        os << opt.m_desc;
        return os;
    }
};

}; // namespace ns

#endif
