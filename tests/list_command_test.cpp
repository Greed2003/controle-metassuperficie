#include "metasurface/binary_input.hpp"
#include "metasurface/command_processor.hpp"
#include "metasurface/controller_state.hpp"
#include "metasurface/line_receiver.hpp"
#include "metasurface/pattern_catalog.hpp"
#include "metasurface/pattern_list.hpp"
#include "metasurface/text_output.hpp"

#include <gtest/gtest.h>
#include <climits>
#include <cstring>
#include <sstream>
#include <string>

namespace {

using metasurface::ControllerState;
using metasurface::LineReceiver;
using metasurface::ReceiveResult;
using metasurface::process_command;

const char kModeFourList[] =
    "M4\nT1: 0000\nT2: 1111\nT3: 1010\nT4: 0101\n"
    "T5: 1100\nT6: 0011\nT7: 1001\nT8: 0110\n"
    "T9: 1000\nT10: 0100\nT11: 0010\nT12: 0001\n";

class StringOutput : public metasurface::TextOutput {
public:
    void write(const char* data, size_t length) override {
        text.append(data, length);
    }
    std::string text;
};

class ListCommandTest : public testing::Test {
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
        for (size_t i = 0; text[i] != '\0'; ++i) {
            result = receiver_.receive(text[i], state_, &output_);
        }
        return result;
    }

    ControllerState state_;
    LineReceiver receiver_;
    StringOutput output_;
};

TEST_F(ListCommandTest, ListsSingleChannelMode) {
    ASSERT_TRUE(state_.select_mode(1));
    ASSERT_TRUE(state_.set_outputs(1));
    ASSERT_TRUE(process_command("LISTA", 5, state_, &output_));
    EXPECT_EQ("M1\nT1: 0\nT2: 1\n", output_.text);
    expect_state(1, 1);
}

TEST_F(ListCommandTest, ListsTwoChannelModeInChannelOrder) {
    ASSERT_TRUE(state_.select_mode(2));
    ASSERT_TRUE(state_.set_outputs(3));
    ASSERT_TRUE(process_command("LISTA", 5, state_, &output_));
    EXPECT_EQ("M2\nT1: 00\nT2: 01\nT3: 10\nT4: 11\n", output_.text);
    expect_state(2, 3);
}

TEST_F(ListCommandTest, ListsFourChannelMode) {
    ASSERT_TRUE(process_command("LISTA", 5, state_, &output_));
    EXPECT_EQ(kModeFourList, output_.text);
    expect_state(4, 5);
}

TEST_F(ListCommandTest, ListsEveryModeWithCorrectPatternsAndWidths) {
    for (int mode = 1; mode <= metasurface::kMaxChannels; ++mode) {
        ASSERT_TRUE(state_.select_mode(mode));
        ASSERT_TRUE(state_.set_outputs(1));
        output_.text.clear();
        ASSERT_TRUE(process_command("LISTA", 5, state_, &output_));
        std::istringstream lines(output_.text);
        std::string line;
        ASSERT_TRUE(static_cast<bool>(std::getline(lines, line)));
        EXPECT_EQ("M" + std::to_string(mode), line);
        for (int table = 1; table <= metasurface::pattern_count(mode); ++table) {
            ASSERT_TRUE(static_cast<bool>(std::getline(lines, line)));
            const size_t separator = line.find(": ");
            ASSERT_NE(std::string::npos, separator);
            EXPECT_EQ("T" + std::to_string(table), line.substr(0, separator));
            const std::string bits = line.substr(separator + 2);
            uint32_t actual = 0;
            uint32_t expected = 0;
            ASSERT_TRUE(metasurface::parse_binary_input(bits.data(), bits.size(), mode, actual));
            ASSERT_TRUE(metasurface::get_pattern(mode, table, expected));
            EXPECT_EQ(expected, actual);
        }
        EXPECT_FALSE(static_cast<bool>(std::getline(lines, line)));
        expect_state(mode, 1);
    }
}

TEST_F(ListCommandTest, PreservesStateWhenRepeated) {
    ASSERT_TRUE(process_command("LISTA", 5, state_, &output_));
    ASSERT_TRUE(process_command("LISTA", 5, state_, &output_));
    EXPECT_EQ(std::string(kModeFourList) + kModeFourList, output_.text);
    expect_state(4, 5);
}

TEST_F(ListCommandTest, RejectsBeforeModeSelectionWithoutOutput) {
    ControllerState initial;
    EXPECT_FALSE(process_command("LISTA", 5, initial, &output_));
    EXPECT_TRUE(output_.text.empty());
    EXPECT_EQ(0, initial.mode());
    EXPECT_EQ(0u, initial.outputs());
}

TEST_F(ListCommandTest, RejectsInvalidModesWithoutOutput) {
    const int invalid[] = {INT_MIN, -1, 0, 33, INT_MAX};
    for (int mode : invalid) {
        EXPECT_FALSE(metasurface::write_pattern_list(mode, output_));
        EXPECT_TRUE(output_.text.empty());
    }
}

TEST_F(ListCommandTest, RejectsMissingOutputWithoutChangingState) {
    EXPECT_FALSE(process_command("LISTA", 5, state_));
    expect_state(4, 5);
}

TEST_F(ListCommandTest, RejectsMalformedInputWithoutOutputOrStateChanges) {
    const char* invalid[] = {
        "", "LIST", "LISTAA", "lista", "Lista", " LISTA", "LISTA ",
        "LISTA1", "LISTA\n", "LISTA\r", "XISTA", "LIS TA"
    };
    for (const char* text : invalid) {
        EXPECT_FALSE(process_command(text, std::strlen(text), state_, &output_));
        EXPECT_TRUE(output_.text.empty());
        expect_state(4, 5);
    }
    const char embedded_null[] = {'L', 'I', '\0', 'T', 'A'};
    EXPECT_FALSE(process_command(embedded_null, sizeof(embedded_null), state_, &output_));
    EXPECT_FALSE(process_command(nullptr, 5, state_, &output_));
    EXPECT_TRUE(output_.text.empty());
    expect_state(4, 5);
}

TEST_F(ListCommandTest, AcceptsBufferWithoutNullTerminator) {
    const char command[] = {'L', 'I', 'S', 'T', 'A'};
    ASSERT_TRUE(process_command(command, sizeof(command), state_, &output_));
    EXPECT_EQ(kModeFourList, output_.text);
    expect_state(4, 5);
}

TEST_F(ListCommandTest, OtherCommandsDoNotWriteLists) {
    ASSERT_TRUE(process_command("M2", 2, state_, &output_));
    expect_state(2, 0);
    ASSERT_TRUE(process_command("T4", 2, state_, &output_));
    expect_state(2, 3);
    ASSERT_TRUE(process_command("10", 2, state_, &output_));
    expect_state(2, 1);
    EXPECT_TRUE(output_.text.empty());
}

TEST_F(ListCommandTest, WaitsForLfBeforeListing) {
    EXPECT_EQ(ReceiveResult::Pending, feed("LI"));
    EXPECT_EQ(ReceiveResult::Pending, feed("STA"));
    EXPECT_TRUE(output_.text.empty());
    expect_state(4, 5);
    EXPECT_EQ(ReceiveResult::Applied, feed("\n"));
    EXPECT_EQ(kModeFourList, output_.text);
    expect_state(4, 5);
}

TEST_F(ListCommandTest, AcceptsCrLfOnlyAfterLf) {
    EXPECT_EQ(ReceiveResult::Pending, feed("LISTA\r"));
    EXPECT_TRUE(output_.text.empty());
    expect_state(4, 5);
    EXPECT_EQ(ReceiveResult::Applied, feed("\n"));
    EXPECT_EQ(kModeFourList, output_.text);
    expect_state(4, 5);
}

TEST_F(ListCommandTest, RecoversAfterInvalidListLine) {
    EXPECT_EQ(ReceiveResult::Rejected, feed("LISTA1\n"));
    EXPECT_TRUE(output_.text.empty());
    expect_state(4, 5);
    EXPECT_EQ(ReceiveResult::Applied, feed("LISTA\n"));
    EXPECT_EQ(kModeFourList, output_.text);
    expect_state(4, 5);
}

TEST_F(ListCommandTest, ListsUpdatedModeAfterModeAndTableCommands) {
    ASSERT_EQ(ReceiveResult::Applied, feed("M1\nT2\nLISTA\n"));
    EXPECT_EQ("M1\nT1: 0\nT2: 1\n", output_.text);
    expect_state(1, 1);
    output_.text.clear();
    ASSERT_EQ(ReceiveResult::Applied, feed("M2\nT4\nLISTA\r\n"));
    EXPECT_EQ("M2\nT1: 00\nT2: 01\nT3: 10\nT4: 11\n", output_.text);
    expect_state(2, 3);
}

}
