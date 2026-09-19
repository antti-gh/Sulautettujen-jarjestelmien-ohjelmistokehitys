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
    EXPECT_EQ(time_parse(time_test5), (59 * 60) + 59); //3599s
 
}
 
TEST(TimeParserTest, TestCaseIncorrectTime) {
 
    char time_test[] = "000077";
    EXPECT_EQ(time_parse(time_test), TIME_VALUE_ERROR);
 
    char time_test2[] = "008800";
    EXPECT_EQ(time_parse(time_test2), TIME_VALUE_ERROR);
 
    char time_test3[] = "250000";
    EXPECT_EQ(time_parse(time_test3), TIME_VALUE_ERROR);
}
//Pituuden ja sisällön tarkistukset
TEST(TimeParserTest, TestStringLen) {
 
    char short_time [] = "12345";
    EXPECT_EQ(time_parse(short_time), TIME_LEN_ERROR);
 
    char long_time [] = "1234567";
    EXPECT_EQ(time_parse(long_time), TIME_LEN_ERROR);
 
    char empty_time [] = "";
    EXPECT_EQ(time_parse(empty_time), TIME_LEN_ERROR);
 
    char non_numeric_time [] = "12A456";
    EXPECT_EQ(time_parse(non_numeric_time), TIME_LEN_ERROR);
}
//Taulukkovirheet NULL
TEST(TimeParserTest, TestNullPointer) {
    EXPECT_EQ(time_parse(NULL), TIME_ARRAY_ERROR);
}
 
TEST(SequenceParserTest, TestCaseCorrectSequence) {
    // Oikein muotoillut komennot
    EXPECT_EQ(sequence_parse("R,1000"), SEQ_OK);
    EXPECT_EQ(sequence_parse("Y,500"), SEQ_OK);
    EXPECT_EQ(sequence_parse("G,2500"), SEQ_OK);
   
    // Pelkät kirjaimet (Toisto ja Debug)
    EXPECT_EQ(sequence_parse("T"), SEQ_OK);
    EXPECT_EQ(sequence_parse("D"), SEQ_OK);
}
 
TEST(SequenceParserTest, TestCaseIncorrectColors) {
    // Väärä kirjain tai pieni kirjain
    EXPECT_EQ(sequence_parse("X,1000"), SEQ_COLOR_ERROR);
    EXPECT_EQ(sequence_parse("B,500"), SEQ_COLOR_ERROR);
    EXPECT_EQ(sequence_parse("r,1000"), SEQ_COLOR_ERROR); // Pienet kirjaimet eivät kelpaa
}
 
TEST(SequenceParserTest, TestCaseIncorrectFormat) {
    // Puuttuva pilkku
    EXPECT_EQ(sequence_parse("R1000"), SEQ_FORMAT_ERROR);
   
    // Ylimääräisiä kirjaimia ajan seassa
    EXPECT_EQ(sequence_parse("R,10A0"), SEQ_TIME_ERROR);
   
    // Aika puuttuu tai on nolla
    EXPECT_EQ(sequence_parse("R,"), SEQ_FORMAT_ERROR);
    EXPECT_EQ(sequence_parse("R,0"), SEQ_TIME_ERROR);
}
 
TEST(SequenceParserTest, TestNullPointerSeq) {
    EXPECT_EQ(sequence_parse(NULL), SEQ_NULL_ERROR);
}
// https://google.github.io/googletest/reference/testing.html
// https://google.github.io/googletest/reference/assertions.html
