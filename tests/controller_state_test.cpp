#include "metasurface/controller_state.hpp"

#include <gtest/gtest.h>

namespace {

using metasurface::ControllerState;

TEST(ControllerStateTest, StartsWithoutSelectedModeAndWithOutputsOff) {
    ControllerState state;

    EXPECT_EQ(0, state.mode());
    EXPECT_EQ(0U, state.outputs());
}

TEST(ControllerStateTest, RejectsOutputsBeforeModeSelection) {
    ControllerState state;

    EXPECT_FALSE(state.set_outputs(0));
    EXPECT_FALSE(state.set_outputs(1));
    EXPECT_EQ(0, state.mode());
    EXPECT_EQ(0U, state.outputs());
}

TEST(ControllerStateTest, AppliesAndClearsOutputsWithinSelectedMode) {
    ControllerState state;
    ASSERT_TRUE(state.select_mode(4));

    ASSERT_TRUE(state.set_outputs(5));
    EXPECT_EQ(4, state.mode());
    EXPECT_EQ(5U, state.outputs());

    ASSERT_TRUE(state.set_outputs(0));
    EXPECT_EQ(4, state.mode());
    EXPECT_EQ(0U, state.outputs());
}

TEST(ControllerStateTest, ClearsOutputsWhenModeChanges) {
    ControllerState state;
    ASSERT_TRUE(state.select_mode(4));
    ASSERT_TRUE(state.set_outputs(15));

    ASSERT_TRUE(state.select_mode(2));
    EXPECT_EQ(2, state.mode());
    EXPECT_EQ(0U, state.outputs());
}

TEST(ControllerStateTest, ClearsOutputsWhenSameModeIsSelectedAgain) {
    ControllerState state;
    ASSERT_TRUE(state.select_mode(4));
    ASSERT_TRUE(state.set_outputs(15));

    ASSERT_TRUE(state.select_mode(4));
    EXPECT_EQ(4, state.mode());
    EXPECT_EQ(0U, state.outputs());
}

TEST(ControllerStateTest, InvalidModePreservesModeAndOutputs) {
    ControllerState state;
    ASSERT_TRUE(state.select_mode(4));
    ASSERT_TRUE(state.set_outputs(5));

    const int invalid_modes[] = {-1, 0, 33};
    for (int mode : invalid_modes) {
        EXPECT_FALSE(state.select_mode(mode));
        EXPECT_EQ(4, state.mode());
        EXPECT_EQ(5U, state.outputs());
    }
}

TEST(ControllerStateTest, RejectsOutputsBeyondSelectedModeWithoutChangingState) {
    ControllerState state;
    ASSERT_TRUE(state.select_mode(4));
    ASSERT_TRUE(state.set_outputs(5));

    EXPECT_FALSE(state.set_outputs(16));
    EXPECT_EQ(4, state.mode());
    EXPECT_EQ(5U, state.outputs());

    EXPECT_FALSE(state.set_outputs(0x80000000UL));
    EXPECT_EQ(4, state.mode());
    EXPECT_EQ(5U, state.outputs());
}

TEST(ControllerStateTest, ModeOneAllowsOnlyFirstChannel) {
    ControllerState state;
    ASSERT_TRUE(state.select_mode(1));

    ASSERT_TRUE(state.set_outputs(1));
    EXPECT_EQ(1U, state.outputs());
    EXPECT_FALSE(state.set_outputs(2));
    EXPECT_EQ(1, state.mode());
    EXPECT_EQ(1U, state.outputs());
}

TEST(ControllerStateTest, ModeThirtyOneRejectsLastChannel) {
    ControllerState state;
    ASSERT_TRUE(state.select_mode(31));
    ASSERT_TRUE(state.set_outputs(0x7FFFFFFFUL));

    EXPECT_FALSE(state.set_outputs(0x80000000UL));
    EXPECT_EQ(31, state.mode());
    EXPECT_EQ(0x7FFFFFFFUL, state.outputs());
}

TEST(ControllerStateTest, ModeThirtyTwoAllowsLastChannelAndAllChannels) {
    ControllerState state;
    ASSERT_TRUE(state.select_mode(32));

    ASSERT_TRUE(state.set_outputs(0x80000000UL));
    EXPECT_EQ(0x80000000UL, state.outputs());

    ASSERT_TRUE(state.set_outputs(0xFFFFFFFFUL));
    EXPECT_EQ(32, state.mode());
    EXPECT_EQ(0xFFFFFFFFUL, state.outputs());
}

}
