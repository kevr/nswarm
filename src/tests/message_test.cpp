#include <gtest/gtest.h>
#include <nswarm/data.hpp>

using namespace ns;

TEST(message_test, json_message)
{
    const uint8_t error_value = 0;
    const uint8_t args_value = 0;

    net::header head(data_type::auth::value, args_value, error_value,
                     action_type::request::value, 0);

    net::json_message js;
    js.update(head);

    EXPECT_EQ(to_type(js.get_type()), net::message::auth);
    EXPECT_EQ(js.head().args(), 0);
    EXPECT_EQ(to_error(js.get_error()), net::error::none);
    EXPECT_EQ(to_action(js.get_action()), net::action::request);
    EXPECT_EQ(js.head().size(), 0);

    json data;
    data["key"] = "test";

    {
        json tmp = data;
        js.update(std::move(tmp));
        EXPECT_EQ(js.head().size(), data.dump().size());
    }

    js.update(head); // reset header back to original 0 size
    js.update(data); // json copy update

    EXPECT_EQ(js.head().size(), data.dump().size());

    // Reconstruct new pieces now that we have valid data to use beforehand
    head = net::header(data_type::auth::value, args_value, error_value,
                       action_type::request::value, data.dump().size());
    js = net::json_message(head, data);

    // Cross reference sizes
    EXPECT_EQ(js.head().size(), data.dump().size());
    EXPECT_EQ(js.head().size(), js.data().size());
    EXPECT_EQ(js.head().size(), js.json().dump().size());

    EXPECT_FALSE(js.has_error());

    match(net::message::deduce(js.get_type()), [](auto t) {
        logi("Type: ", decltype(t)::human);
    });

    match(net::action::deduce(js.get_action()), [](auto act) {
        logi("Action: ", decltype(act)::human);
    });

    uint16_t value = 0;
    std::cout << "Enter a message value: ";
    ASSERT_TRUE(std::cin >> value);

    match(net::message::deduce(value), [](auto msg) {
        logi(decltype(msg)::human);
    });
    // Auto is required to catch possible bad
    match(net::error::deduce(js.get_error()), [](auto err) {
        logi(decltype(err)::human);
    });

    logi("JSON: ", js.get_json().dump());
}

