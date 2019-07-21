#include <nswarm/logging.hpp>
#include <nswarm/variant.hpp>
#include <gtest/gtest.h>

using ns::match;

TEST(variant_test, variant_matches)
{
    std::variant<int, double> v;
    v = 1.0;

    auto result = match(
        v, [](int a) { return static_cast<double>(a + 1); },
        [](double a) { return a; });

    logi("Matched: ", result);
}

TEST(variant_test, variant_matches_string)
{
    std::variant<int, std::string> v;
    v = "test";

    auto result = match(
        v, [](int i) { return std::to_string(i); },
        [](const std::string &s) { return s; });
    logi("Matched: ", result);

    v = 1;
    result = match(
        v, [](int i) { return std::to_string(i); },
        [](const std::string &s) { return s; });
    logi("Matched: ", result);
}

// Some test tagging

struct ip4_tag {
    static constexpr int value = 4;
};

struct ip6_tag {
    static constexpr int value = 6;
};

TEST(variant_test, variant_tagged_structs)
{
    std::variant<ip4_tag, ip6_tag> ip_tag;

    ip_tag = ip4_tag();
    auto result = match(
        ip_tag, [](ip4_tag) { return ip4_tag::value; },
        [](ip6_tag) { return ip6_tag::value; });
    logi("Tag: ", result);

    ip_tag = ip6_tag();
    result = match(
        ip_tag, [](ip4_tag) { return ip4_tag::value; },
        [](ip6_tag) { return ip6_tag::value; });
    logi("Tag: ", result);
}

