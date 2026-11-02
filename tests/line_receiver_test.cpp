#include "metasurface/line_receiver.hpp"
#include "metasurface/controller_state.hpp"

#include <gtest/gtest.h>

namespace {

using metasurface::ControllerState;
using metasurface::LineReceiver;
using metasurface::ReceiveResult;

class LineReceiverTest : public testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(state_.select_mode(4));
        ASSERT_TRUE(state_.set_outputs(5));
    }

    ReceiveResult feed(const char* text) {
        ReceiveResult result = ReceiveResult::Pending;
        for (size_t index = 0; text[index] != '\0'; ++index) {
            result = receiver_.receive(text[index], state_);
        }
        return result;
    }

    void expect_state(int mode, uint32_t outputs) {
        EXPECT_EQ(mode, state_.mode());
        EXPECT_EQ(outputs, state_.outputs());
    }

    ControllerState state_;
    LineReceiver receiver_;
};

TEST_F(LineReceiverTest, WaitsForLfBeforeApplyingMode) {
    EXPECT_EQ(ReceiveResult::Pending, receiver_.receive('M', state_));
    expect_state(4, 5);
    EXPECT_EQ(ReceiveResult::Pending, receiver_.receive('2', state_));
    expect_state(4, 5);
    EXPECT_EQ(ReceiveResult::Applied, receiver_.receive('\n', state_));
    expect_state(2, 0);
}

TEST_F(LineReceiverTest, AppliesBinaryCommandOnlyAtLf) {
    EXPECT_EQ(ReceiveResult::Pending, feed("01"));
    expect_state(4, 5);
    EXPECT_EQ(ReceiveResult::Pending, feed("01"));
    expect_state(4, 5);
    EXPECT_EQ(ReceiveResult::Applied, receiver_.receive('\n', state_));
    expect_state(4, 10);
}

TEST_F(LineReceiverTest, AcceptsCrLfOnlyAfterLf) {
    EXPECT_EQ(ReceiveResult::Pending, feed("M2\r"));
    expect_state(4, 5);
    EXPECT_EQ(ReceiveResult::Applied, receiver_.receive('\n', state_));
    expect_state(2, 0);
    EXPECT_EQ(ReceiveResult::Applied, feed("10\r\n"));
    expect_state(2, 1);
}

TEST_F(LineReceiverTest, ProcessesSuccessiveCommands) {
    EXPECT_EQ(ReceiveResult::Applied, feed("M2\n"));
    expect_state(2, 0);
    EXPECT_EQ(ReceiveResult::Applied, feed("11\r\n"));
    expect_state(2, 3);
    EXPECT_EQ(ReceiveResult::Applied, feed("M1\n"));
    expect_state(1, 0);
    EXPECT_EQ(ReceiveResult::Applied, feed("1\n"));
    expect_state(1, 1);
}

TEST_F(LineReceiverTest, IgnoresEmptyLines) {
    EXPECT_EQ(ReceiveResult::Pending, feed("\n"));
    expect_state(4, 5);
    EXPECT_EQ(ReceiveResult::Pending, feed("\r\n"));
    expect_state(4, 5);
    EXPECT_EQ(ReceiveResult::Applied, feed("0101\n"));
    expect_state(4, 10);
}

TEST_F(LineReceiverTest, RejectsInvalidCommandsAndRecovers) {
    EXPECT_EQ(ReceiveResult::Rejected, feed("M33\n"));
    expect_state(4, 5);
    EXPECT_EQ(ReceiveResult::Rejected, feed("111x\n"));
    expect_state(4, 5);
    EXPECT_EQ(ReceiveResult::Applied, feed("0101\n"));
    expect_state(4, 10);
}

TEST_F(LineReceiverTest, AcceptsMaximumLengthWithLf) {
    ASSERT_EQ(ReceiveResult::Applied, feed("M32\n"));
    EXPECT_EQ(ReceiveResult::Applied,
        feed("11111111" "11111111" "11111111" "11111111\n"));
    expect_state(32, UINT32_MAX);
}

TEST_F(LineReceiverTest, AcceptsMaximumLengthWithCrLf) {
    ASSERT_EQ(ReceiveResult::Applied, feed("M32\r\n"));
    EXPECT_EQ(ReceiveResult::Pending,
        feed("11111111" "11111111" "11111111" "11111111\r"));
    expect_state(32, 0);
    EXPECT_EQ(ReceiveResult::Applied, receiver_.receive('\n', state_));
    expect_state(32, UINT32_MAX);
}

TEST_F(LineReceiverTest, DiscardsOverlongLineUntilLfAndRecovers) {
    ASSERT_EQ(ReceiveResult::Applied, feed("M32\n"));
    ASSERT_TRUE(state_.set_outputs(5));
    EXPECT_EQ(ReceiveResult::Pending,
        feed("11111111" "11111111" "11111111" "11111111"));
    expect_state(32, 5);
    EXPECT_EQ(ReceiveResult::Pending, receiver_.receive('1', state_));
    expect_state(32, 5);
    EXPECT_EQ(ReceiveResult::Pending, feed("M1\r"));
    expect_state(32, 5);
    EXPECT_EQ(ReceiveResult::Rejected, receiver_.receive('\n', state_));
    expect_state(32, 5);
    EXPECT_EQ(ReceiveResult::Applied, feed("M1\n"));
    expect_state(1, 0);
}

TEST_F(LineReceiverTest, RejectsMisplacedCarriageReturnsAndRecovers) {
    EXPECT_EQ(ReceiveResult::Rejected, feed("10\r10\n"));
    expect_state(4, 5);
    EXPECT_EQ(ReceiveResult::Rejected, feed("M2\r\r\n"));
    expect_state(4, 5);
    EXPECT_EQ(ReceiveResult::Applied, feed("M2\n"));
    expect_state(2, 0);
}

TEST_F(LineReceiverTest, RejectsEmbeddedNullAndRecovers) {
    const char text[] = {'0', '1', '\0', '1'};
    for (char character : text) {
        EXPECT_EQ(ReceiveResult::Pending, receiver_.receive(character, state_));
        expect_state(4, 5);
    }
    EXPECT_EQ(ReceiveResult::Rejected, receiver_.receive('\n', state_));
    expect_state(4, 5);
    EXPECT_EQ(ReceiveResult::Applied, feed("0101\n"));
    expect_state(4, 10);
}

TEST_F(LineReceiverTest, RejectsBinaryLineUntilModeIsSelected) {
    state_ = ControllerState();
    EXPECT_EQ(ReceiveResult::Rejected, feed("1010\n"));
    expect_state(0, 0);
    EXPECT_EQ(ReceiveResult::Applied, feed("M4\n"));
    expect_state(4, 0);
    EXPECT_EQ(ReceiveResult::Applied, feed("1010\n"));
    expect_state(4, 5);
}

}
