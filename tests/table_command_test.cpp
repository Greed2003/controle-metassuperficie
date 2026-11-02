#include "metasurface/table_command.hpp"
#include "metasurface/command_processor.hpp"
#include "metasurface/controller_state.hpp"
#include "metasurface/line_receiver.hpp"
#include "metasurface/pattern_catalog.hpp"

#include <gtest/gtest.h>
#include <cstring>
#include <string>

namespace {

using metasurface::ControllerState;
using metasurface::LineReceiver;
using metasurface::ReceiveResult;
using metasurface::apply_table_command;
using metasurface::process_command;

class TableCommandTest : public testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(state_.select_mode(4));
        ASSERT_TRUE(state_.set_outputs(5));
    }

    void expect_state(int mode, uint32_t outputs) {
        EXPECT_EQ(mode, state_.mode());
        EXPECT_EQ(outputs, state_.outputs());
    }

    ReceiveResult feed(const char* text) {
        ReceiveResult result = ReceiveResult::Pending;
        for (size_t index = 0; text[index] != '\0'; ++index) {
            result = receiver_.receive(text[index], state_);
        }
        return result;
    }

    ControllerState state_;
    LineReceiver receiver_;
};

TEST_F(TableCommandTest, RejectsBeforeModeSelection) {
    ControllerState initial;
    EXPECT_FALSE(apply_table_command("T1", 2, initial));
    EXPECT_EQ(0, initial.mode());
    EXPECT_EQ(0u, initial.outputs());
}

TEST_F(TableCommandTest, AppliesEveryCatalogTable) {
    for (int mode = 1; mode <= metasurface::kMaxChannels; ++mode) {
        ASSERT_TRUE(state_.select_mode(mode));
        for (int table = 1; table <= metasurface::pattern_count(mode); ++table) {
            const std::string command = "T" + std::to_string(table);
            uint32_t expected = 0;
            ASSERT_TRUE(metasurface::get_pattern(mode, table, expected));
            ASSERT_TRUE(apply_table_command(command.data(), command.size(), state_));
            expect_state(mode, expected);
        }
    }
}

TEST_F(TableCommandTest, RejectsTablesOutsideCurrentMode) {
    for (int mode = 1; mode <= metasurface::kMaxChannels; ++mode) {
        ASSERT_TRUE(state_.select_mode(mode));
        ASSERT_TRUE(state_.set_outputs(1));
        const int invalid[] = {0, metasurface::pattern_count(mode) + 1, 99};
        for (int table : invalid) {
            const std::string command = "T" + std::to_string(table);
            EXPECT_FALSE(apply_table_command(command.data(), command.size(), state_));
            expect_state(mode, 1);
        }
    }
}

TEST_F(TableCommandTest, ReplacesOutputsWithoutChangingMode) {
    ASSERT_TRUE(state_.set_outputs(15));
    ASSERT_TRUE(apply_table_command("T5", 2, state_));
    expect_state(4, 3);
    ASSERT_TRUE(apply_table_command("T12", 3, state_));
    expect_state(4, 8);
}

TEST_F(TableCommandTest, ClearsOutputsWithFirstTable) {
    ASSERT_TRUE(apply_table_command("T1", 2, state_));
    expect_state(4, 0);
}

TEST_F(TableCommandTest, ReapplyingSameTableKeepsOutputs) {
    ASSERT_TRUE(apply_table_command("T5", 2, state_));
    ASSERT_TRUE(apply_table_command("T5", 2, state_));
    expect_state(4, 3);
}

TEST_F(TableCommandTest, SupportsAllThirtyTwoChannels) {
    ASSERT_TRUE(state_.select_mode(32));
    ASSERT_TRUE(apply_table_command("T2", 2, state_));
    expect_state(32, 0xffffffffu);
    ASSERT_TRUE(apply_table_command("T9", 2, state_));
    expect_state(32, 0x80000001u);
    ASSERT_TRUE(apply_table_command("T12", 3, state_));
    expect_state(32, 0xffff0000u);
}

TEST_F(TableCommandTest, RejectsMalformedCommandsWithoutChangingState) {
    const char* invalid[] = {
        "t2", "M2", "X2", "12", " T2", "T01", "T00", "T02",
        "T-1", "T+1", "Tx", "T1x", "T 2", "T2 ", "T2\n", "T2\r"
    };
    for (const char* command : invalid) {
        EXPECT_FALSE(apply_table_command(command, std::strlen(command), state_));
        expect_state(4, 5);
    }
}

TEST_F(TableCommandTest, RejectsNullAndWrongLengths) {
    EXPECT_FALSE(apply_table_command(nullptr, 2, state_));
    EXPECT_FALSE(apply_table_command("", 0, state_));
    EXPECT_FALSE(apply_table_command("T", 1, state_));
    EXPECT_FALSE(apply_table_command("T123", 4, state_));
    EXPECT_FALSE(apply_table_command("T", static_cast<size_t>(-1), state_));
    expect_state(4, 5);
}

TEST_F(TableCommandTest, RejectsEmbeddedNull) {
    const char command[] = {'T', '1', '\0'};
    EXPECT_FALSE(apply_table_command(command, sizeof(command), state_));
    expect_state(4, 5);
}

TEST_F(TableCommandTest, AcceptsBufferWithoutNullTerminator) {
    const char command[] = {'T', '1', '2'};
    ASSERT_TRUE(apply_table_command(command, sizeof(command), state_));
    expect_state(4, 8);
}

TEST_F(TableCommandTest, ProcessesTablesAndManualCommands) {
    ControllerState initial;
    EXPECT_FALSE(process_command("T1", 2, initial));
    EXPECT_EQ(0, initial.mode());
    EXPECT_EQ(0u, initial.outputs());
    ASSERT_TRUE(process_command("T5", 2, state_));
    expect_state(4, 3);
    ASSERT_TRUE(process_command("0101", 4, state_));
    expect_state(4, 10);
    ASSERT_TRUE(process_command("T2", 2, state_));
    expect_state(4, 15);
    ASSERT_TRUE(process_command("M1", 2, state_));
    expect_state(1, 0);
    EXPECT_FALSE(process_command("T3", 2, state_));
    expect_state(1, 0);
    ASSERT_TRUE(process_command("T2", 2, state_));
    expect_state(1, 1);
}

TEST_F(TableCommandTest, AppliesLinesOnlyAfterLf) {
    EXPECT_EQ(ReceiveResult::Pending, feed("T5\r"));
    expect_state(4, 5);
    EXPECT_EQ(ReceiveResult::Applied, feed("\n"));
    expect_state(4, 3);
    EXPECT_EQ(ReceiveResult::Applied, feed("T12\n"));
    expect_state(4, 8);
}

TEST_F(TableCommandTest, RecoversAfterInvalidTableLine) {
    EXPECT_EQ(ReceiveResult::Rejected, feed("T13\n"));
    expect_state(4, 5);
    EXPECT_EQ(ReceiveResult::Applied, feed("T5\n"));
    expect_state(4, 3);
    ASSERT_EQ(ReceiveResult::Applied, feed("M2\n"));
    expect_state(2, 0);
    EXPECT_EQ(ReceiveResult::Rejected, feed("T5\n"));
    expect_state(2, 0);
    EXPECT_EQ(ReceiveResult::Applied, feed("T4\r\n"));
    expect_state(2, 3);
}

}
