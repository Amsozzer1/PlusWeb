#include <gtest/gtest.h>

TEST(SimpleTest, BasicAssertion) {
    EXPECT_EQ(2 + 2, 4);
    EXPECT_TRUE(true);
    EXPECT_FALSE(false);
}

TEST(SimpleTest, StringComparison) {
    std::string hello = "hello";
    EXPECT_EQ(hello, "hello");
    EXPECT_NE(hello, "world");
}
