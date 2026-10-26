#include "metasurface/mode.hpp"

#include <gtest/gtest.h>
#include <climits>

namespace {

class ValidModeTest : public testing::TestWithParam<int> {};

TEST_P(ValidModeTest, AcceptsSupportedMode) {
    EXPECT_TRUE(metasurface::is_valid_mode(GetParam()));
}

INSTANTIATE_TEST_SUITE_P(
    SupportedModes,
    ValidModeTest,
    testing::Range(1, 33)
);

class InvalidModeTest : public testing::TestWithParam<int> {};

TEST_P(InvalidModeTest, RejectsUnsupportedMode) {
    EXPECT_FALSE(metasurface::is_valid_mode(GetParam()));
}

INSTANTIATE_TEST_SUITE_P(
    UnsupportedModes,
    InvalidModeTest,
    testing::Values(INT_MIN, -1, 0, 33, 256, INT_MAX)
);

}
