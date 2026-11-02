#include "metasurface/command_processor.hpp"
#include "metasurface/controller_state.hpp"

#include <gtest/gtest.h>
#include <cstring>

namespace {

using metasurface::ControllerState;
using metasurface::process_command;

TEST(CommandProcessorTest, RejectsBinaryInputBeforeModeSelection) {
    ControllerState state;
    EXPECT_FALSE(process_command("1010", 4, state));
    EXPECT_EQ(0, state.mode());
    EXPECT_EQ(0u, state.outputs());
}

TEST(CommandProcessorTest, AppliesSuccessiveBinaryCommands) {
    ControllerState state;
    ASSERT_TRUE(process_command("M4", 2, state));
    EXPECT_EQ(4, state.mode());
    EXPECT_EQ(0u, state.outputs());

    ASSERT_TRUE(process_command("1010", 4, state));
    EXPECT_EQ(5u, state.outputs());
    ASSERT_TRUE(process_command("0101", 4, state));
    EXPECT_EQ(10u, state.outputs());
    ASSERT_TRUE(process_command("0000", 4, state));
    EXPECT_EQ(0u, state.outputs());
    EXPECT_EQ(4, state.mode());
}

TEST(CommandProcessorTest, ModeChangeClearsOutputsAndUpdatesRequiredLength) {
    ControllerState state;
    ASSERT_TRUE(process_command("M4", 2, state));
    ASSERT_TRUE(process_command("1111", 4, state));
    ASSERT_TRUE(process_command("M2", 2, state));
    EXPECT_EQ(2, state.mode());
    EXPECT_EQ(0u, state.outputs());

    EXPECT_FALSE(process_command("1010", 4, state));
    EXPECT_EQ(2, state.mode());
    EXPECT_EQ(0u, state.outputs());
    ASSERT_TRUE(process_command("10", 2, state));
    EXPECT_EQ(1u, state.outputs());
}

TEST(CommandProcessorTest, RepeatingModeClearsOutputs) {
    ControllerState state;
    ASSERT_TRUE(process_command("M4", 2, state));
    ASSERT_TRUE(process_command("1111", 4, state));
    ASSERT_TRUE(process_command("M4", 2, state));
    EXPECT_EQ(4, state.mode());
    EXPECT_EQ(0u, state.outputs());
}

TEST(CommandProcessorTest, InvalidCommandsPreserveStateAndAllowRecovery) {
    ControllerState state;
    ASSERT_TRUE(process_command("M4", 2, state));
    ASSERT_TRUE(process_command("1010", 4, state));

    const char* inputs[] = {"M33", "M01", "111x", "111", "X4", "M4\n", "1010\n"};
    for (const char* text : inputs) {
        SCOPED_TRACE(text);
        EXPECT_FALSE(process_command(text, std::strlen(text), state));
        EXPECT_EQ(4, state.mode());
        EXPECT_EQ(5u, state.outputs());
    }

    ASSERT_TRUE(process_command("0101", 4, state));
    EXPECT_EQ(4, state.mode());
    EXPECT_EQ(10u, state.outputs());
}

TEST(CommandProcessorTest, SupportsFirstAndLastChannelsInModeThirtyTwo) {
    ControllerState state;
    ASSERT_TRUE(process_command("M32", 3, state));
    const char text[] = "10000000" "00000000" "00000000" "00000001";
    ASSERT_TRUE(process_command(text, sizeof(text) - 1, state));
    EXPECT_EQ(32, state.mode());
    EXPECT_EQ(0x80000001u, state.outputs());
}

TEST(CommandProcessorTest, SupportsSingleChannelMode) {
    ControllerState state;
    ASSERT_TRUE(process_command("M1", 2, state));
    ASSERT_TRUE(process_command("1", 1, state));
    EXPECT_EQ(1u, state.outputs());
    EXPECT_FALSE(process_command("10", 2, state));
    EXPECT_EQ(1u, state.outputs());
    ASSERT_TRUE(process_command("0", 1, state));
    EXPECT_EQ(0u, state.outputs());
    EXPECT_EQ(1, state.mode());
}

TEST(CommandProcessorTest, RejectsNullAndEmptyInputWithoutChangingState) {
    ControllerState state;
    ASSERT_TRUE(process_command("M4", 2, state));
    ASSERT_TRUE(process_command("1010", 4, state));
    EXPECT_FALSE(process_command(nullptr, 4, state));
    EXPECT_EQ(4, state.mode());
    EXPECT_EQ(5u, state.outputs());
    EXPECT_FALSE(process_command("", 0, state));
    EXPECT_EQ(4, state.mode());
    EXPECT_EQ(5u, state.outputs());
}

TEST(CommandProcessorTest, AcceptsBuffersWithoutNullTerminator) {
    ControllerState state;
    const char mode[] = {'M', '4'};
    const char binary[] = {'1', '0', '1', '0'};
    ASSERT_TRUE(process_command(mode, sizeof(mode), state));
    ASSERT_TRUE(process_command(binary, sizeof(binary), state));
    EXPECT_EQ(4, state.mode());
    EXPECT_EQ(5u, state.outputs());
}

}
