#ifndef NS_PROCESS_HPP
#define NS_PROCESS_HPP

#include "logging.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <regex>
#include <tuple>
#include <unistd.h>
#include <vector>

namespace ns
{
inline uint64_t bytes_in_use()
{
    trace();

    uint64_t bytes = 0;

    pid_t pid = getpid();

    std::string pid_s = std::to_string(pid);
    std::string command("pmap -x " + std::to_string(pid));

    FILE *proc = popen(command.c_str(), "r");
    if (!proc)
        throw std::invalid_argument("unable to open process '" + command + "'");

    std::string output;
    char buf[256];
    while (fgets(buf, sizeof(buf), proc) != nullptr) {
        output.append(buf);
    }

    pclose(proc);

    std::vector<std::string> lines;
    std::size_t pos = output.find("\n");
    std::size_t beg = 0;
    while (pos != std::string::npos) {
        lines.emplace_back(output.substr(beg, pos - beg));
        beg = pos + 1;
        pos = output.find("\n", beg);
    }

    auto path = lines[0].substr(lines[0].rfind(" "));
    path = std::string(basename((char *)path.c_str()));
    logd("path: ", path);
    lines.erase(lines.begin());

    auto get_fields = [](const auto &str)
        -> std::optional<std::tuple<std::string, std::string, std::string,
                                    std::string, std::string, std::string>> {
        std::regex re(
            R"(^([0-9a-fA-F]+)\s+(\d+)\s+(\d+)\s+(\d+)\s+?([rwxspd-]{5})\s(.+)$)");
        //       abcd01234        16       16      0      rw-x-- sensor_test
        //       1                2        3       4      5      6

        std::smatch sm;
        if (!std::regex_match(str.begin(), str.end(), sm, re)) {
            return std::nullopt;
        }

        return std::make_tuple(sm[1], sm[2], sm[3], sm[4], sm[5], sm[6]);
    };

    for (auto &line : lines) {
        auto pos = line.rfind(" ");
        if (pos == std::string::npos)
            continue;

        pos = line.find(path, pos - 1);
        if (pos != std::string::npos) {

            auto [addr, kbUse, kbSSI, kbRes, permissions, name] =
                *get_fields(line);

            bytes += (uint64_t)std::stoul(kbUse);

            logd("Memory in use: ", kbUse, "kB");
            // We have some  bytes!
        }
    }

    bytes *= 1000;
    return bytes;
}

}; // namespace ns

#endif
