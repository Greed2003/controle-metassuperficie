#include "metasurface/pattern_catalog.hpp"
#include "metasurface/binary_input.hpp"
#include <gtest/gtest.h>
#include <climits>
#include <cstring>
#include <initializer_list>
#include <set>

namespace {

using metasurface::get_pattern;
using metasurface::pattern_count;

void expect_patterns(int mode, std::initializer_list<const char*> patterns) {
    SCOPED_TRACE(mode);
    EXPECT_EQ(static_cast<int>(patterns.size()), pattern_count(mode));
    int table = 1;
    for (const char* text : patterns) {
        SCOPED_TRACE(table);
        uint32_t expected = 0;
        uint32_t outputs = UINT32_MAX;
        ASSERT_TRUE(metasurface::parse_binary_input(text, std::strlen(text), mode, expected));
        ASSERT_TRUE(get_pattern(mode, table, outputs));
        EXPECT_EQ(expected, outputs);
        ++table;
    }
}

TEST(PatternCatalogTest, HasExpectedCountsForEveryMode) {
    EXPECT_EQ(2, pattern_count(1));
    EXPECT_EQ(4, pattern_count(2));
    EXPECT_EQ(8, pattern_count(3));
    for (int mode = 4; mode <= 32; ++mode) {
        EXPECT_EQ(12, pattern_count(mode)) << mode;
    }
    int total = 0;
    for (int mode = 1; mode <= 32; ++mode) {
        total += pattern_count(mode);
    }
    EXPECT_EQ(362, total);
}

TEST(PatternCatalogTest, RejectsInvalidModesWithoutChangingOutputs) {
    for (int mode : {INT_MIN, -1, 0, 33, 256, INT_MAX}) {
        SCOPED_TRACE(mode);
        uint32_t outputs = UINT32_MAX;
        EXPECT_EQ(0, pattern_count(mode));
        EXPECT_FALSE(get_pattern(mode, 1, outputs));
        EXPECT_EQ(UINT32_MAX, outputs);
    }
}

TEST(PatternCatalogTest, RejectsInvalidTablesWithoutChangingOutputs) {
    for (int mode = 1; mode <= 32; ++mode) {
        SCOPED_TRACE(mode);
        for (int table : {INT_MIN, -1, 0, pattern_count(mode) + 1, INT_MAX}) {
            SCOPED_TRACE(table);
            uint32_t outputs = 0x80000001u;
            EXPECT_FALSE(get_pattern(mode, table, outputs));
            EXPECT_EQ(0x80000001u, outputs);
        }
    }
}

TEST(PatternCatalogTest, HasDistinctPatternsWithinEveryMode) {
    for (int mode = 1; mode <= 32; ++mode) {
        SCOPED_TRACE(mode);
        std::set<uint32_t> patterns;
        for (int table = 1; table <= pattern_count(mode); ++table) {
            uint32_t outputs = 0;
            ASSERT_TRUE(get_pattern(mode, table, outputs));
            EXPECT_TRUE(patterns.insert(outputs).second) << table;
        }
    }
}

TEST(PatternCatalogTest, KeepsChannelsOutsideModeOff) {
    for (int mode = 1; mode < 32; ++mode) {
        SCOPED_TRACE(mode);
        for (int table = 1; table <= pattern_count(mode); ++table) {
            uint32_t outputs = UINT32_MAX;
            ASSERT_TRUE(get_pattern(mode, table, outputs));
            EXPECT_EQ(0u, outputs >> mode) << table;
        }
    }
}

TEST(PatternCatalogTest, MatchesSmallModeCatalogs) {
    expect_patterns(1, {
        "0", "1"
    });
    expect_patterns(2, {
        "00", "01", "10", "11"
    });
    expect_patterns(3, {
        "000", "001", "010", "011", "100", "101", "110", "111"
    });
}

TEST(PatternCatalogTest, MatchesModeFourCatalog) {
    expect_patterns(4, {
        "0000", "1111", "1010", "0101", "1100", "0011", "1001", "0110", "1000",
        "0100", "0010", "0001"
    });
}

TEST(PatternCatalogTest, MatchesModeFiveCatalog) {
    expect_patterns(5, {
        "00000", "11111", "10101", "01010", "11001", "00110", "11110", "00001",
        "10001", "00100", "11100", "00011"
    });
}

TEST(PatternCatalogTest, MatchesModeSixCatalog) {
    expect_patterns(6, {
        "000000", "111111", "101010", "010101", "110011", "001100", "111100",
        "000011", "100001", "111000", "000111", "100000"
    });
}

TEST(PatternCatalogTest, MatchesModeSevenAndEightCatalogs) {
    expect_patterns(7, {
        "0000000", "1111111", "1010101", "0101010", "1100110", "0011001", "1111000",
        "0000111", "1000001", "0001000", "1000000", "0100000"
    });
    expect_patterns(8, {
        "00000000", "11111111", "10101010", "01010101", "11001100", "00110011",
        "11110000", "00001111", "10000001", "00011000", "10000000", "01000000"
    });
}

TEST(PatternCatalogTest, MatchesModeThirtyOneCatalog) {
    expect_patterns(31, {
        "0000000000000000000000000000000", "1111111111111111111111111111111",
        "1010101010101010101010101010101", "0101010101010101010101010101010",
        "1100110011001100110011001100110", "0011001100110011001100110011001",
        "1111000011110000111100001111000", "0000111100001111000011110000111",
        "1000000000000000000000000000001", "0000000000000001000000000000000",
        "1111111111111111000000000000000", "0000000000000000111111111111111"
    });
}

TEST(PatternCatalogTest, MatchesModeThirtyTwoCatalog) {
    expect_patterns(32, {
        "00000000000000000000000000000000", "11111111111111111111111111111111",
        "10101010101010101010101010101010", "01010101010101010101010101010101",
        "11001100110011001100110011001100", "00110011001100110011001100110011",
        "11110000111100001111000011110000", "00001111000011110000111100001111",
        "10000000000000000000000000000001", "00000000000000011000000000000000",
        "11111111111111110000000000000000", "00000000000000001111111111111111"
    });
}

}
