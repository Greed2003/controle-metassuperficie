#include "metasurface/mode_command.hpp"
#include "metasurface/controller_state.hpp"

#include <gtest/gtest.h>
#include <cstdio>
#include <cstring>

namespace {

using metasurface::ControllerState;
using metasurface::apply_mode_command;

class ModeCommandTest : public testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(state_.select_mode(4));
        ASSERT_TRUE(state_.set_outputs(5));
    }

    void expect_rejected(const char* text, size_t length) {
        EXPECT_FALSE(apply_mode_command(text, length, state_));
        EXPECT_EQ(4, state_.mode());
        EXPECT_EQ(5u, state_.outputs());
    }

    ControllerState state_;
};

TEST_F(ModeCommandTest, AcceptsAllSupportedModes) {
    for (int mode = 1; mode <= 32; ++mode) {
        SCOPED_TRACE(mode);
        char text[4] = {};
        const int length = std::snprintf(text, sizeof(text), "M%d", mode);
        ControllerState state;
        ASSERT_TRUE(apply_mode_command(text, static_cast<size_t>(length), state));
        EXPECT_EQ(mode, state.mode());
        EXPECT_EQ(0u, state.outputs());
    }
}

TEST_F(ModeCommandTest, ChangesModeAndClearsOutputs) {
    ASSERT_TRUE(apply_mode_command("M2", 2, state_));
    EXPECT_EQ(2, state_.mode());
    EXPECT_EQ(0u, state_.outputs());
}

TEST_F(ModeCommandTest, ReselectsModeAndClearsOutputs) {
    ASSERT_TRUE(apply_mode_command("M4", 2, state_));
    EXPECT_EQ(4, state_.mode());
    EXPECT_EQ(0u, state_.outputs());
}

TEST_F(ModeCommandTest, RejectsOutOfRangeModes) {
    const char* inputs[] = {"M0", "M33", "M99"};
    for (const char* text : inputs) {
        SCOPED_TRACE(text);
        expect_rejected(text, std::strlen(text));
    }
}

TEST_F(ModeCommandTest, RejectsIncorrectPrefix) {
    const char* inputs[] = {"m4", "T4", "14", " M4"};
    for (const char* text : inputs) {
        SCOPED_TRACE(text);
        expect_rejected(text, std::strlen(text));
    }
}

TEST_F(ModeCommandTest, RejectsNonDigitCharacters) {
    const char* inputs[] = {"M-1", "M+1", "M1x", "Mx", "M 4", "M4 ", "M4\n"};
    for (const char* text : inputs) {
        SCOPED_TRACE(text);
        expect_rejected(text, std::strlen(text));
    }
}

TEST_F(ModeCommandTest, RejectsLeadingZeros) {
    const char* inputs[] = {"M01", "M00", "M04"};
    for (const char* text : inputs) {
        SCOPED_TRACE(text);
        expect_rejected(text, std::strlen(text));
    }
}

TEST_F(ModeCommandTest, RejectsWrongLengths) {
    const char* inputs[] = {"", "M", "M123"};
    for (const char* text : inputs) {
        SCOPED_TRACE(text);
        expect_rejected(text, std::strlen(text));
    }
}

TEST_F(ModeCommandTest, RejectsNullInput) {
    expect_rejected(nullptr, 2);
}

TEST_F(ModeCommandTest, AcceptsBufferWithoutNullTerminator) {
    const char text[] = {'M', '3', '2'};
    ASSERT_TRUE(apply_mode_command(text, sizeof(text), state_));
    EXPECT_EQ(32, state_.mode());
    EXPECT_EQ(0u, state_.outputs());
}

TEST_F(ModeCommandTest, RejectsEmbeddedNull) {
    const char text[] = {'M', '1', '\0'};
    expect_rejected(text, sizeof(text));
}

TEST_F(ModeCommandTest, KeepsUnselectedStateOnInvalidCommand) {
    ControllerState state;
    EXPECT_FALSE(apply_mode_command("M33", 3, state));
    EXPECT_EQ(0, state.mode());
    EXPECT_EQ(0u, state.outputs());
}

}
