#include <gtest/gtest.h>
#include <nswarm/dispatcher.hpp>

TEST(dispatcher_test, dispatcher)
{
    ns::task_dispatcher<int> dispatcher;

    dispatcher.push("abcd", [](auto ptr, ns::json_message data) {
        EXPECT_EQ(ptr, nullptr);
    });

    // Cannot put the same key twice
    EXPECT_ANY_THROW(
        dispatcher.push("abcd", [](auto ptr, ns::json_message data) {
            EXPECT_EQ(ptr, nullptr);
        }));

    auto result = dispatcher.pop("abcd");
    auto [task_id, f] = *result;
    f(nullptr, ns::json_message(0));

    // Already popped this key
    EXPECT_EQ(dispatcher.pop("abcd"), std::nullopt);
}
