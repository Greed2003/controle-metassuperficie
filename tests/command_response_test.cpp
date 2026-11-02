#include "metasurface/command_processor.hpp"
#include "metasurface/controller_state.hpp"
#include "metasurface/line_receiver.hpp"
#include "metasurface/pattern_catalog.hpp"
#include "metasurface/text_output.hpp"

#include <gtest/gtest.h>
#include <cstring>
#include <string>

namespace {

using metasurface::ReceiveResult;
using metasurface::process_command;

class ResponseOutput : public metasurface::TextOutput {
public:
    void write(const char* text, size_t length) override { value.append(text, length); }
    std::string value;
};

class CommandResponseTest : public testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(state_.select_mode(4));
        ASSERT_TRUE(state_.set_outputs(5));
    }

    void expect_command(const std::string& command, bool accepted,
                        const std::string& reply, int mode, uint32_t outputs) {
        output_.value.clear();
        EXPECT_EQ(accepted, process_command(command.data(), command.size(), state_, &output_));
        EXPECT_EQ(reply, output_.value);
        EXPECT_EQ(mode, state_.mode());
        EXPECT_EQ(outputs, state_.outputs());
    }

    ReceiveResult feed(const std::string& text) {
        ReceiveResult result = ReceiveResult::Pending;
        for (char character : text) {
            result = receiver_.receive(character, state_, &output_);
        }
        return result;
    }

    metasurface::ControllerState state_;
    metasurface::LineReceiver receiver_;
    ResponseOutput output_;
};

TEST_F(CommandResponseTest, ConfirmsEveryModeAndReselection) {
    for (int mode = 1; mode <= 32; ++mode) {
        const std::string command = "M" + std::to_string(mode);
        const std::string reply = "OK: modo " + command + "; saidas desligadas.\n";
        expect_command(command, true, reply, mode, 0);
        ASSERT_TRUE(state_.set_outputs(1));
        expect_command(command, true, reply, mode, 0);
    }
}

TEST_F(CommandResponseTest, ConfirmsEveryCatalogTable) {
    for (int mode = 1; mode <= 32; ++mode) {
        ASSERT_TRUE(state_.select_mode(mode));
        for (int table = 1; table <= metasurface::pattern_count(mode); ++table) {
            uint32_t expected = 0;
            ASSERT_TRUE(metasurface::get_pattern(mode, table, expected));
            expect_command("T" + std::to_string(table), true, "OK: tabela aplicada.\n", mode, expected);
        }
    }
}

TEST_F(CommandResponseTest, ConfirmsBinaryInputInEveryMode) {
    for (int mode = 1; mode <= 32; ++mode) {
        ASSERT_TRUE(state_.select_mode(mode));
        std::string bits(mode, '0');
        bits[0] = '1';
        expect_command(bits, true, "OK: saidas atualizadas.\n", mode, 1);
        expect_command(std::string(mode, '0'), true, "OK: saidas atualizadas.\n", mode, 0);
    }
}

TEST_F(CommandResponseTest, RequiresModeBeforeDataOrList) {
    metasurface::ControllerState initial;
    for (const char* command : {"1", "T1", "LISTA"}) {
        output_.value.clear();
        EXPECT_FALSE(process_command(command, std::strlen(command), initial, &output_));
        EXPECT_EQ("ERRO: selecione um modo de M1 a M32.\n", output_.value);
        EXPECT_EQ(0, initial.mode());
        EXPECT_EQ(0u, initial.outputs());
    }
}

TEST_F(CommandResponseTest, ExplainsInvalidModesWithoutChangingState) {
    for (const char* command : {"M", "M0", "M33", "M99", "M01", "M-1", "M4X"}) {
        expect_command(command, false, "ERRO: modo invalido; use M1 a M32.\n", 4, 5);
    }
}

TEST_F(CommandResponseTest, ReportsTableRangeForEveryMode) {
    for (int mode = 1; mode <= 32; ++mode) {
        ASSERT_TRUE(state_.select_mode(mode));
        ASSERT_TRUE(state_.set_outputs(1));
        const int count = metasurface::pattern_count(mode);
        const std::string reply = "ERRO: tabela invalida; use T1 a T" + std::to_string(count) + ".\n";
        expect_command("T" + std::to_string(count + 1), false, reply, mode, 1);
        expect_command("T0", false, reply, mode, 1);
    }
}

TEST_F(CommandResponseTest, RejectsMalformedTablesWithCurrentRange) {
    for (const char* command : {"T", "T00", "T01", "T-1", "T1X", "T 1"}) {
        expect_command(command, false, "ERRO: tabela invalida; use T1 a T12.\n", 4, 5);
    }
}

TEST_F(CommandResponseTest, ReportsExpectedBinaryLengthForEveryMode) {
    for (int mode = 1; mode <= 32; ++mode) {
        ASSERT_TRUE(state_.select_mode(mode));
        ASSERT_TRUE(state_.set_outputs(1));
        const std::string reply = "ERRO: entrada binaria invalida; envie exatamente "
            + std::to_string(mode) + (mode == 1 ? " bit (0 ou 1).\n" : " bits (0 ou 1).\n");
        expect_command(std::string(mode + 1, '1'), false, reply, mode, 1);
        if (mode > 1) {
            expect_command(std::string(mode - 1, '1'), false, reply, mode, 1);
        }
    }
}

TEST_F(CommandResponseTest, RejectsInvalidBinaryCharactersAndEmbeddedNull) {
    const std::string reply = "ERRO: entrada binaria invalida; envie exatamente 4 bits (0 ou 1).\n";
    for (const char* command : {"10x0", "10 0", "10\r0", "10\n0", "1020"}) {
        expect_command(command, false, reply, 4, 5);
    }
    const char invalid[] = {'1', '0', '\0', '1'};
    expect_command(std::string(invalid, sizeof(invalid)), false, reply, 4, 5);
}

TEST_F(CommandResponseTest, ExplainsUnknownEmptyAndNullCommands) {
    const std::string reply = "ERRO: comando invalido; use M1 a M32, Tn, LISTA ou bits.\n";
    for (const char* command : {"", "m4", "t1", "lista", "LISTA1", "OFF", " 1010", "2010"}) {
        expect_command(command, false, reply, 4, 5);
    }
    output_.value.clear();
    EXPECT_FALSE(process_command(nullptr, 4, state_, &output_));
    EXPECT_EQ(reply, output_.value);
    EXPECT_EQ(4, state_.mode());
    EXPECT_EQ(5u, state_.outputs());
}

TEST_F(CommandResponseTest, KeepsProcessingWithoutAnOutputDestination) {
    ASSERT_TRUE(process_command("M2", 2, state_));
    ASSERT_TRUE(process_command("T4", 2, state_));
    ASSERT_TRUE(process_command("10", 2, state_));
    EXPECT_FALSE(process_command("T5", 2, state_));
    EXPECT_FALSE(process_command("LISTA", 5, state_));
    EXPECT_EQ(2, state_.mode());
    EXPECT_EQ(1u, state_.outputs());
}

TEST_F(CommandResponseTest, WaitsForLfWithFragmentedCrLf) {
    EXPECT_EQ(ReceiveResult::Pending, feed("M"));
    EXPECT_EQ(ReceiveResult::Pending, feed("2\r"));
    EXPECT_TRUE(output_.value.empty());
    EXPECT_EQ(4, state_.mode());
    EXPECT_EQ(5u, state_.outputs());
    EXPECT_EQ(ReceiveResult::Applied, feed("\n"));
    EXPECT_EQ("OK: modo M2; saidas desligadas.\n", output_.value);
    EXPECT_EQ(2, state_.mode());
    EXPECT_EQ(0u, state_.outputs());
}

TEST_F(CommandResponseTest, IgnoresEmptyLinesWithoutReply) {
    EXPECT_EQ(ReceiveResult::Pending, feed("\n\r\n\n"));
    EXPECT_TRUE(output_.value.empty());
    EXPECT_EQ(4, state_.mode());
    EXPECT_EQ(5u, state_.outputs());
}

TEST_F(CommandResponseTest, RepliesOnceAfterDiscardingAnOverlongLineAndRecovers) {
    EXPECT_EQ(ReceiveResult::Pending, feed(std::string(33, '1') + "M1"));
    EXPECT_TRUE(output_.value.empty());
    EXPECT_EQ(ReceiveResult::Rejected, feed("\n"));
    EXPECT_EQ("ERRO: linha invalida; use ate 32 caracteres e termine com LF ou CRLF.\n", output_.value);
    EXPECT_EQ(4, state_.mode());
    EXPECT_EQ(5u, state_.outputs());
    output_.value.clear();
    EXPECT_EQ(ReceiveResult::Applied, feed("0000\n"));
    EXPECT_EQ("OK: saidas atualizadas.\n", output_.value);
    EXPECT_EQ(0u, state_.outputs());
}

TEST_F(CommandResponseTest, RejectsMisplacedCarriageReturnsAndRecovers) {
    for (const char* line : {"M\r2\n", "M2\r\r\n", "\rM2\n"}) {
        output_.value.clear();
        EXPECT_EQ(ReceiveResult::Rejected, feed(line));
        EXPECT_EQ("ERRO: linha invalida; use ate 32 caracteres e termine com LF ou CRLF.\n", output_.value);
        EXPECT_EQ(4, state_.mode());
        EXPECT_EQ(5u, state_.outputs());
    }
    output_.value.clear();
    EXPECT_EQ(ReceiveResult::Applied, feed("T1\n"));
    EXPECT_EQ("OK: tabela aplicada.\n", output_.value);
    EXPECT_EQ(0u, state_.outputs());
}

TEST_F(CommandResponseTest, RejectsEmbeddedNullLineAndRecovers) {
    const char invalid[] = {'1', '0', '\0', '1', '\n'};
    EXPECT_EQ(ReceiveResult::Rejected, feed(std::string(invalid, sizeof(invalid))));
    EXPECT_EQ("ERRO: entrada binaria invalida; envie exatamente 4 bits (0 ou 1).\n", output_.value);
    EXPECT_EQ(5u, state_.outputs());
    output_.value.clear();
    EXPECT_EQ(ReceiveResult::Applied, feed("0101\n"));
    EXPECT_EQ("OK: saidas atualizadas.\n", output_.value);
    EXPECT_EQ(10u, state_.outputs());
}

TEST_F(CommandResponseTest, AcceptsThirtyTwoBitsWithCrLf) {
    ASSERT_TRUE(state_.select_mode(32));
    EXPECT_EQ(ReceiveResult::Applied, feed(std::string(32, '1') + "\r\n"));
    EXPECT_EQ("OK: saidas atualizadas.\n", output_.value);
    EXPECT_EQ(UINT32_MAX, state_.outputs());
}

TEST_F(CommandResponseTest, OrdersRepliesAndListsAcrossSuccessesAndErrors) {
    EXPECT_EQ(ReceiveResult::Rejected, feed("M1\nT2\nLISTA\n0\nT3\n"));
    EXPECT_EQ("OK: modo M1; saidas desligadas.\nOK: tabela aplicada.\n"
        "M1\nT1: 0\nT2: 1\nOK: saidas atualizadas.\n"
        "ERRO: tabela invalida; use T1 a T2.\n", output_.value);
    EXPECT_EQ(1, state_.mode());
    EXPECT_EQ(0u, state_.outputs());
}

}
