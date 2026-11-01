#include "metasurface/binary_input.hpp"

#include <gtest/gtest.h>

namespace {

using metasurface::parse_binary_input;

TEST(BinaryInputTest, AcceptsSingleChannelValues) {
    uint32_t outputs = 7;
    ASSERT_TRUE(parse_binary_input("0", 1, 1, outputs));
    EXPECT_EQ(0u, outputs);
    ASSERT_TRUE(parse_binary_input("1", 1, 1, outputs));
    EXPECT_EQ(1u, outputs);
}

TEST(BinaryInputTest, MapsCharactersToChannelsInOrder) {
    uint32_t outputs = 0;
    ASSERT_TRUE(parse_binary_input("1010", 4, 4, outputs));
    EXPECT_EQ(5u, outputs);
    ASSERT_TRUE(parse_binary_input("0101", 4, 4, outputs));
    EXPECT_EQ(10u, outputs);
}

TEST(BinaryInputTest, PreservesLeadingZeros) {
    uint32_t outputs = 0;
    ASSERT_TRUE(parse_binary_input("0001", 4, 4, outputs));
    EXPECT_EQ(8u, outputs);
}

TEST(BinaryInputTest, ClearsAllOutputs) {
    uint32_t outputs = UINT32_MAX;
    ASSERT_TRUE(parse_binary_input("0000", 4, 4, outputs));
    EXPECT_EQ(0u, outputs);
}

TEST(BinaryInputTest, AcceptsAllThirtyTwoChannelsOn) {
    const char text[] = "11111111" "11111111" "11111111" "11111111";
    uint32_t outputs = 0;
    ASSERT_TRUE(parse_binary_input(text, sizeof(text) - 1, 32, outputs));
    EXPECT_EQ(UINT32_MAX, outputs);
}

TEST(BinaryInputTest, MapsFirstAndLastChannelsInModeThirtyTwo) {
    const char first[] = "10000000" "00000000" "00000000" "00000000";
    const char last[] = "00000000" "00000000" "00000000" "00000001";
    uint32_t outputs = 0;
    ASSERT_TRUE(parse_binary_input(first, sizeof(first) - 1, 32, outputs));
    EXPECT_EQ(1u, outputs);
    ASSERT_TRUE(parse_binary_input(last, sizeof(last) - 1, 32, outputs));
    EXPECT_EQ(UINT32_C(0x80000000), outputs);
}

TEST(BinaryInputTest, AcceptsBufferWithoutNullTerminator) {
    const char text[] = {'1', '0', '1', '0'};
    uint32_t outputs = 0;
    ASSERT_TRUE(parse_binary_input(text, sizeof(text), 4, outputs));
    EXPECT_EQ(5u, outputs);
}

TEST(BinaryInputTest, RejectsNullInputWithoutChangingOutputs) {
    uint32_t outputs = 7;
    EXPECT_FALSE(parse_binary_input(nullptr, 4, 4, outputs));
    EXPECT_EQ(7u, outputs);
}

TEST(BinaryInputTest, RejectsInvalidModesWithoutChangingOutputs) {
    const int modes[] = {-1, 0, 33};
    for (int mode : modes) {
        SCOPED_TRACE(mode);
        uint32_t outputs = 7;
        EXPECT_FALSE(parse_binary_input("1010", 4, mode, outputs));
        EXPECT_EQ(7u, outputs);
    }
}

TEST(BinaryInputTest, RejectsWrongLengthsWithoutChangingOutputs) {
    const size_t lengths[] = {0, 3, 5};
    for (size_t length : lengths) {
        SCOPED_TRACE(length);
        uint32_t outputs = 7;
        EXPECT_FALSE(parse_binary_input("10101", length, 4, outputs));
        EXPECT_EQ(7u, outputs);
    }
}

TEST(BinaryInputTest, RejectsInvalidCharactersWithoutChangingOutputs) {
    const char* inputs[] = {"10x0", "1012", " 101", "101 ", "10,1", "10\t1", "101\n"};
    for (const char* text : inputs) {
        SCOPED_TRACE(text);
        uint32_t outputs = 7;
        EXPECT_FALSE(parse_binary_input(text, 4, 4, outputs));
        EXPECT_EQ(7u, outputs);
    }
}

TEST(BinaryInputTest, RejectsEmbeddedNullWithoutChangingOutputs) {
    const char text[] = {'1', '0', '\0', '1'};
    uint32_t outputs = 7;
    EXPECT_FALSE(parse_binary_input(text, sizeof(text), 4, outputs));
    EXPECT_EQ(7u, outputs);
}

TEST(BinaryInputTest, RejectsInvalidLastCharacterAtMaximumLength) {
    const char text[] = "11111111" "11111111" "11111111" "1111111x";
    uint32_t outputs = 7;
    EXPECT_FALSE(parse_binary_input(text, sizeof(text) - 1, 32, outputs));
    EXPECT_EQ(7u, outputs);
}

}
