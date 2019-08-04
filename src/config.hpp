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

#include <nswarm/logging.hpp>
#include <nswarm/util.hpp>

// c++ standard library
#include <boost/program_options.hpp>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
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

        logi("Options:");
        for (auto &kv : m_vm) {
            std::string s = kv.second.as<std::string>();
            if (m_required.find(kv.first) == m_required.end() && s == "")
                s = "true";
            logi("    ", kv.first, " = ", s);
        }

        for (const auto &required : m_required) {
            if (!exists(required)) {
                std::cout << "error: required key missing '" << required << "'"
                          << std::endl
                          << std::endl;
                m_valid = false;
                return;
            }
        }
    }

    // ini configuration file
    void parse_config(const std::string &path)
    {
        std::ifstream ifs(path.c_str());
        if (!ifs.is_open()) {
            m_valid = false;
            return;
        }

        // Try to parse it
        try {
            boost::program_options::store(
                boost::program_options::parse_config_file(ifs, m_desc), m_vm);
            boost::program_options::notify(m_vm);
            m_valid = true;
        } catch (std::exception &e) {
            std::cout << "error: " << e.what() << std::endl << std::endl;
            m_valid = false;
        }

        // Make sure we close up; don't rely on ifs destructor
        ifs.close();
    }

    template <typename T>
    program_options &add_required_option(const std::string &name,
                                         std::string help)
    {
        help.append(" (required)");
        m_desc.add_options()(name.c_str(), boost::program_options::value<T>(),
                             help.c_str());
        m_required.emplace(name);
        return *this;
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
        std::string out("usage: " + m_executable + " [-hvxd] [--log arg]");
        for (const auto &required : m_required)
            out.append(" --" + required + " arg");
        return out;
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
        add_option("help,h", "Print this help message");
        add_option("debug,v", "Enable debug logging");
        add_option("trace,x", "Enable trace logging (includes debug)");
        add_option("daemon,d", "Daemonize process");
        add_option<std::string>("log", "Path to optional logfile");
    }

private:
    bool m_valid = true;
    std::string m_executable;

    boost::program_options::variables_map m_vm;
    boost::program_options::options_description m_desc;

    std::set<std::string> m_required;

    friend std::ostream &operator<<(std::ostream &os,
                                    const ns::program_options &opt)
    {
        os << opt.m_desc;
        return os;
    }
};

template <typename... Args>
inline void parse_configs(ns::program_options &opt, Args &&... args)
{
    auto configs = util::any_file(std::forward<Args>(args)...);
    for (const auto &config : configs) {
        opt.parse_config(config);
        logi("loaded configuration file: ", config);
    }
}

}; // namespace ns

#endif
