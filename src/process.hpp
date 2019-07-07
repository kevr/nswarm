#ifndef NS_PROCESS_HPP
#define NS_PROCESS_HPP

#include "logging.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <unistd.h>
#include <vector>

namespace ns
{
inline uint64_t bytes_in_use()
{
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
    // logi("process output:\n", output);

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

    for (auto &line : lines) {
        auto pos = line.find(path);
        if (pos != std::string::npos) {
            logd("found record");
            // We have some  bytes!
        }
    }

    return bytes;
}

}; // namespace ns

#endif
