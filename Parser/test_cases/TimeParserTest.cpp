#include <gtest/gtest.h>
#include "../TimeParser.h"

// Test suite: TimeParserTest
TEST(TimeParserTest, TestCaseCorrectTime) {

    // Note that this test fails on purpose!!

    // Test with correct time string
    char time_test[] = "000005";
    EXPECT_EQ(time_parse(time_test), 5);

    char time_test2[] = "000105";
    EXPECT_EQ(time_parse(time_test2), 65);

    char time_test3[] = "001000";
    EXPECT_EQ(time_parse(time_test3), 600);

    char time_test4[] = "120130";
    EXPECT_EQ(time_parse(time_test4), 90);
    
    char time_test5[] = "235959";
    EXPECT_EQ(time_parse(time_test5), (59 * 60) +59);


}

TEST(TimeParserTest, TestCaseIncorrectTime) {

    char time_test[] = "000077";
    EXPECT_EQ(time_parse(time_test), TIME_VALUE_ERROR);

    char time_test2[] = "008800";
    EXPECT_EQ(time_parse(time_test2), TIME_VALUE_ERROR);

    char time_test3[] = "250000";
    EXPECT_EQ(time_parse(time_test3), TIME_VALUE_ERROR);
}

TEST(TimeParserTest, TestStringLen) {

    char short_time[] = "12345";
    EXPECT_EQ(time_parse(short_time), TIME_LEN_ERROR);

    char long_time[] = "1234567";
    EXPECT_EQ(time_parse(long_time), TIME_LEN_ERROR);

    char empty_time[] = "";
    EXPECT_EQ(time_parse(empty_time), TIME_LEN_ERROR);


}

TEST(TimeParserTest, TestuNullPointer) {
    EXPECT_EQ(time_parse(NULL), TIME_ARRAY_ERROR);
}

// https://google.github.io/googletest/reference/testing.html
// https://google.github.io/googletest/reference/assertions.html
