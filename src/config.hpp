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
    program_options() = default;

    program_options(int argc, const char *argv[])
    {
        parse(argc, argv);
    }

    program_options(const program_options &opt)
        : m_executable(opt.m_executable)
    {
    } // Copy constructor

    void operator=(const program_options &opt)
    {
        m_executable = opt.m_executable;
    }

    program_options(program_options &&opt)
        : m_executable(std::move(opt.m_executable))
    {
    } // Move constructor

    void operator=(program_options &&opt)
    {
        m_executable = std::move(opt.m_executable);
    }

    // Parse arguments in here. The rest of this object should
    // just provided metadata about the program arguments.
    void parse(int argc, const char *argv[])
    {
        m_executable = basename(const_cast<char *>(argv[0]));
    }

    const std::string &name() const
    {
        return m_executable;
    }

private:
    std::string m_executable;
};

}; // namespace ns

#endif
