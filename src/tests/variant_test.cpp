#include <gtest/gtest.h>
#include <nswarm/auth.hpp>
#include <nswarm/data.hpp>
#include <nswarm/logging.hpp>
#include <nswarm/variant.hpp>

using ns::match;

TEST(variant_test, variant_matches)
{
    std::variant<int, double> v;
    v = 1.0;

    auto result = match(
        v,
        [](int a) {
            return static_cast<double>(a + 1);
        },
        [](double a) {
            return a;
        });

    logi("Matched: ", result);
}

TEST(variant_test, variant_matches_string)
{
    std::variant<int, std::string> v;
    v = "test";

    auto result = match(
        v,
        [](int i) {
            return std::to_string(i);
        },
        [](const std::string &s) {
            return s;
        });
    logi("Matched: ", result);

    v = 1;
    result = match(
        v,
        [](int i) {
            return std::to_string(i);
        },
        [](const std::string &s) {
            return s;
        });
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
        ip_tag,
        [](ip4_tag) {
            return ip4_tag::value;
        },
        [](ip6_tag) {
            return ip6_tag::value;
        });
    logi("Tag: ", result);

    ip_tag = ip6_tag();
    result = match(
        ip_tag,
        [](ip4_tag) {
            return ip4_tag::value;
        },
        [](ip6_tag) {
            return ip6_tag::value;
        });
    logi("Tag: ", result);
}

TEST(variant_test, auth_type_variant)
{
    /* We should fix this for new API
    std::string key("abcd");
    ns::auth<ns::auth_type::key, ns::action_type::request> auth(key);

    bool good = false;

    ns::match(
        ns::data_type::deduce(auth.type()),
        [&](ns::data_type::auth) {
            good = true;
        },
        [](ns::data_type::implement) {
        },
        [](ns::data_type::subscribe) {
        },
        [](ns::data_type::task) {
        },
        [](auto &&) {
        });
    EXPECT_TRUE(good);

    good = false;
    ns::match(
        // The criteria
        ns::auth_type::deduce(auth.params()),

        // The matching functions
        [&](ns::auth_type::key) {
            good = true;
        },
        [](auto &&) {
        });

    EXPECT_TRUE(good);

    good = false;

    ns::match(
        // The criteria
        ns::action_type::deduce(auth.action()),

        // The matching functions
        [&](ns::action_type::request) {
            good = true;
        },
        [&](ns::action_type::response) {
            good = false;
        },
        [](auto &&) {
        });

    EXPECT_TRUE(good);

    ns::value::auth_value invalid_value = (ns::value::auth_value)66;
    EXPECT_ANY_THROW(ns::match(ns::auth_type::deduce(invalid_value),
                               [&](ns::auth_type::key) {
                               }));
    */
}

