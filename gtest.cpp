#include <gtest/gtest.h>

std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> result;
    size_t start = 0;
    size_t end = s.find(delim);
    while (end != std::string::npos) {
        result.push_back(s.substr(start, end - start));
        start = end + 1;
        end = s.find(delim, start);
    }
    result.push_back(s.substr(start, s.size() - start));
    return result;
}

// Test splitting a normal string
TEST(SplitFunction, NormalString) {
    std::string testString = "one,two,three";
    char delim = ',';
    std::vector<std::string> expected = {"one", "two", "three"};
    auto result = split(testString, delim);
    EXPECT_EQ(result, expected);
}

// Test splitting an empty string
TEST(SplitFunction, EmptyString) {
    std::string testString = "";
    char delim = ',';
    std::vector<std::string> expected = {""};
    auto result = split(testString, delim);
    EXPECT_EQ(result, expected);
}

// Test string with consecutive delimiters
TEST(SplitFunction, ConsecutiveDelimiters) {
    std::string testString = "one,,two";
    char delim = ',';
    std::vector<std::string> expected = {"one", "", "two"};
    auto result = split(testString, delim);
    EXPECT_EQ(result, expected);
}

// Test string that ends with a delimiter
TEST(SplitFunction, EndsWithDelimiter) {
    std::string testString = "one,two,";
    char delim = ',';
    std::vector<std::string> expected = {"one", "two", ""};
    auto result = split(testString, delim);
    EXPECT_EQ(result, expected);
}

// Test string that starts with a delimiter
TEST(SplitFunction, StartsWithDelimiter) {
    std::string testString = ",one,two";
    char delim = ',';
    std::vector<std::string> expected = {"", "one", "two"};
    auto result = split(testString, delim);
    EXPECT_EQ(result, expected);
}

// Main function to run the tests
int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
