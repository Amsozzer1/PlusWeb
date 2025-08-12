#include <gtest/gtest.h>
#include "../include/PlusWeb/utils.h"

class UtilsTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(UtilsTest, SplitBasicPath) {
    std::vector<std::string> result = Utils::split("/users/123", "/");
    
    ASSERT_EQ(result.size(), 2);
    // EXPECT_EQ(result[0], "");      // Empty because path starts with /
    EXPECT_EQ(result[0], "users");
    EXPECT_EQ(result[1], "123");
}

TEST_F(UtilsTest, SplitSingleSegment) {
    std::vector<std::string> result = Utils::split("users", "/");
    
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], "users");
}

TEST_F(UtilsTest, SplitEmptyString) {
    std::vector<std::string> result = Utils::split("", "/");
    EXPECT_TRUE(result.empty());
}

TEST_F(UtilsTest, SplitParameterPath) {
    std::vector<std::string> result = Utils::split("/users/:id/posts", "/");
    
    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "users");
    EXPECT_EQ(result[1], ":id");
    EXPECT_EQ(result[2], "posts");
}

TEST_F(UtilsTest, SplitMultipleSlashes) {
    std::vector<std::string> result = Utils::split("/users//123/", "/");
    
    // Should handle empty segments
    EXPECT_TRUE(result.size() >= 2);
    EXPECT_EQ(result[0], "users");
    EXPECT_EQ(result[1], "123");
}