#include "metasurface/controller_application.hpp"
#include "metasurface/digital_output.hpp"
#include "metasurface/pattern_catalog.hpp"
#include "metasurface/text_output.hpp"

#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <string>
#include <vector>

namespace {

using metasurface::ReceiveResult;

const char* const kInvalidLine =
    "ERRO: linha invalida; use ate 32 caracteres e termine com LF ou CRLF.\n";

class RecordingIo : public metasurface::DigitalOutput, public metasurface::TextOutput {
public:
    RecordingIo() { levels.fill(true); }

    void write(uint8_t pin, bool high) override {
        ASSERT_GE(pin, 22);
        ASSERT_LE(pin, 53);
        levels[pin] = high;
        order += 'W';
    }

    void configure_output(uint8_t pin) override {
        EXPECT_GE(pin, 22);
        EXPECT_LE(pin, 53);
        EXPECT_EQ(0u, mask());
        order += 'C';
    }

    void write(const char* text, size_t length) override {
        reply.append(text, length);
        levels_at_reply.push_back(mask());
        order += 'R';
    }

    uint32_t mask() const {
        uint32_t result = 0;
        for (int channel = 0; channel < 32; ++channel) {
            if (levels[22 + channel]) {
                result |= uint32_t(1) << channel;
            }
        }
        return result;
    }

    void clear() {
        order.clear();
        reply.clear();
        levels_at_reply.clear();
    }

    std::array<bool, 54> levels;
    std::string order;
    std::string reply;
    std::vector<uint32_t> levels_at_reply;
};

ReceiveResult feed(metasurface::ControllerApplication& app, const std::string& text) {
    ReceiveResult result = ReceiveResult::Pending;
    for (char character : text) {
        result = app.receive(character);
    }
    return result;
}

class ControllerApplicationTest : public testing::Test {
protected:
    ControllerApplicationTest() : app_(io_, &io_) {}

    void SetUp() override {
        app_.initialize();
        io_.clear();
    }

    void expect_change(const std::string& line, int mode, uint32_t mask,
                       const std::string& reply) {
        io_.clear();
        ASSERT_EQ(ReceiveResult::Applied, feed(app_, line));
        EXPECT_EQ(mode, app_.state().mode());
        EXPECT_EQ(mask, app_.state().outputs());
        EXPECT_EQ(mask, io_.mask());
        EXPECT_EQ(reply, io_.reply);
        EXPECT_EQ(std::string(32, 'W'), io_.order.substr(0, 32));
        EXPECT_EQ(32u, io_.order.find('R'));
        EXPECT_EQ(32, std::count(io_.order.begin(), io_.order.end(), 'W'));
        EXPECT_EQ(std::string::npos, io_.order.find('C'));
        for (uint32_t observed : io_.levels_at_reply) {
            EXPECT_EQ(mask, observed);
        }
        for (int pin = 0; pin < 22; ++pin) {
            EXPECT_TRUE(io_.levels[pin]);
        }
    }

    void expect_rejected(const std::string& line) {
        const int mode = app_.state().mode();
        const uint32_t mask = app_.state().outputs();
        io_.clear();
        EXPECT_EQ(ReceiveResult::Rejected, feed(app_, line));
        EXPECT_EQ(mode, app_.state().mode());
        EXPECT_EQ(mask, app_.state().outputs());
        EXPECT_EQ(mask, io_.mask());
        EXPECT_EQ(0u, io_.reply.find("ERRO:"));
        EXPECT_EQ(std::string::npos, io_.order.find_first_of("WC"));
    }

    RecordingIo io_;
    metasurface::ControllerApplication app_;
};

TEST_F(ControllerApplicationTest, ConstructionAndInputBeforeInitializationHaveNoEffects) {
    RecordingIo io;
    metasurface::ControllerApplication app(io, &io);
    app.discard_line();
    EXPECT_EQ(ReceiveResult::Pending, feed(app, "M32\nT2\n"));
    EXPECT_TRUE(io.order.empty());
    EXPECT_TRUE(io.reply.empty());
    EXPECT_EQ(UINT32_MAX, io.mask());
    EXPECT_EQ(0, app.state().mode());
    EXPECT_EQ(0u, app.state().outputs());
    app.initialize();
    EXPECT_EQ(ReceiveResult::Applied, feed(app, "M1\n"));
}

TEST_F(ControllerApplicationTest, InitializesAllLowBeforeConfiguringAnyOutput) {
    app_.initialize();
    EXPECT_EQ(std::string(32, 'W') + std::string(32, 'C'), io_.order);
    EXPECT_EQ(0u, io_.mask());
    EXPECT_EQ(0, app_.state().mode());
    EXPECT_EQ(0u, app_.state().outputs());
    EXPECT_TRUE(io_.reply.empty());
}

TEST_F(ControllerApplicationTest, ReinitializationClearsStatePartialInputAndDiscard) {
    for (bool discard : {false, true}) {
        ASSERT_EQ(ReceiveResult::Pending, feed(app_, "M4\n1111\nM"));
        if (discard) {
            app_.discard_line();
        }
        io_.clear();
        app_.initialize();
        EXPECT_EQ(std::string(32, 'W') + std::string(32, 'C'), io_.order);
        EXPECT_EQ(0, app_.state().mode());
        EXPECT_EQ(0u, app_.state().outputs());
        EXPECT_EQ(0u, io_.mask());
        EXPECT_TRUE(io_.reply.empty());
        expect_change("M2\n", 2, 0, "OK: modo M2; saidas desligadas.\n");
    }
}

TEST_F(ControllerApplicationTest, WaitsForLfAndAcceptsFragmentedCrLf) {
    EXPECT_EQ(ReceiveResult::Pending, feed(app_, "M4\r"));
    EXPECT_TRUE(io_.order.empty());
    expect_change("\n", 4, 0, "OK: modo M4; saidas desligadas.\n");
    io_.clear();
    EXPECT_EQ(ReceiveResult::Pending, feed(app_, "01"));
    EXPECT_EQ(ReceiveResult::Pending, feed(app_, "01\r"));
    EXPECT_TRUE(io_.order.empty());
    expect_change("\n", 4, 10, "OK: saidas atualizadas.\n");
}

TEST_F(ControllerApplicationTest, EveryModeSelectionAndReselectionClearsPinsBeforeReply) {
    for (int mode = 1; mode <= 32; ++mode) {
        ASSERT_EQ(ReceiveResult::Applied, feed(app_, "M32\nT2\n"));
        const std::string command = "M" + std::to_string(mode);
        const std::string reply = "OK: modo " + command + "; saidas desligadas.\n";
        expect_change(command + "\n", mode, 0, reply);
        ASSERT_EQ(ReceiveResult::Applied, feed(app_, "T2\n"));
        expect_change(command + "\n", mode, 0, reply);
    }
}

TEST_F(ControllerApplicationTest, BinaryCommandsApplyEachChannelInEveryModeBeforeReply) {
    for (int mode = 1; mode <= 32; ++mode) {
        ASSERT_EQ(ReceiveResult::Applied, feed(app_, "M" + std::to_string(mode) + "\n"));
        for (int channel = 0; channel < mode; ++channel) {
            std::string bits(mode, '0');
            bits[channel] = '1';
            expect_change(bits + "\r\n", mode, uint32_t(1) << channel,
                          "OK: saidas atualizadas.\n");
        }
        expect_change(std::string(mode, '1') + "\n", mode, UINT32_MAX >> (32 - mode),
                      "OK: saidas atualizadas.\n");
    }
}

TEST_F(ControllerApplicationTest, AppliesEveryCatalogTableBeforeReply) {
    for (int mode = 1; mode <= 32; ++mode) {
        ASSERT_EQ(ReceiveResult::Applied, feed(app_, "M" + std::to_string(mode) + "\n"));
        for (int table = 1; table <= metasurface::pattern_count(mode); ++table) {
            uint32_t mask = 0;
            ASSERT_TRUE(metasurface::get_pattern(mode, table, mask));
            expect_change("T" + std::to_string(table) + "\n", mode, mask,
                          "OK: tabela aplicada.\n");
        }
    }
}

TEST_F(ControllerApplicationTest, RepeatedValidCommandsStillApplyBeforeReply) {
    ASSERT_EQ(ReceiveResult::Applied, feed(app_, "M4\nT5\n"));
    expect_change("T5\n", 4, 3, "OK: tabela aplicada.\n");
    expect_change("1100\n", 4, 3, "OK: saidas atualizadas.\n");
    expect_change("1100\n", 4, 3, "OK: saidas atualizadas.\n");
}

TEST_F(ControllerApplicationTest, ListPreservesPinsAndState) {
    ASSERT_EQ(ReceiveResult::Applied, feed(app_, "M1\n1\n"));
    io_.clear();
    EXPECT_EQ(ReceiveResult::Applied, feed(app_, "LISTA\n"));
    EXPECT_EQ("M1\nT1: 0\nT2: 1\n", io_.reply);
    EXPECT_EQ(std::string::npos, io_.order.find_first_of("WC"));
    EXPECT_EQ(1, app_.state().mode());
    EXPECT_EQ(1u, app_.state().outputs());
    EXPECT_EQ(1u, io_.mask());
}

TEST_F(ControllerApplicationTest, RejectedCommandsPreservePinsAndState) {
    ASSERT_EQ(ReceiveResult::Applied, feed(app_, "M4\n0101\n"));
    for (const char* line : {"M0\n", "M33\n", "M01\n", "T13\n", "T0\n",
                             "101\n", "0102\n", "X\n", "LISTA1\n"}) {
        expect_rejected(line);
    }
    expect_rejected(std::string("10\0" "1\n", 5));
}

TEST_F(ControllerApplicationTest, RequiresModeBeforeDataOrList) {
    for (const char* line : {"1\n", "T1\n", "LISTA\n"}) {
        expect_rejected(line);
        EXPECT_EQ("ERRO: selecione um modo de M1 a M32.\n", io_.reply);
    }
}

TEST_F(ControllerApplicationTest, EmptyLinesHaveNoEffects) {
    ASSERT_EQ(ReceiveResult::Applied, feed(app_, "M4\n0101\n"));
    io_.clear();
    EXPECT_EQ(ReceiveResult::Pending, feed(app_, "\n\r\n\n"));
    EXPECT_TRUE(io_.order.empty());
    EXPECT_TRUE(io_.reply.empty());
    EXPECT_EQ(10u, io_.mask());
    EXPECT_EQ(10u, app_.state().outputs());
}

TEST_F(ControllerApplicationTest, OperatesWithoutTextOutput) {
    RecordingIo io;
    metasurface::ControllerApplication app(io);
    app.initialize();
    EXPECT_EQ(ReceiveResult::Applied, feed(app, "M4\nT5\n0101\n"));
    EXPECT_EQ(10u, io.mask());
    io.clear();
    EXPECT_EQ(ReceiveResult::Rejected, feed(app, "LISTA\n"));
    app.discard_line();
    EXPECT_EQ(ReceiveResult::Rejected, feed(app, "M1\n"));
    EXPECT_TRUE(io.order.empty());
    EXPECT_EQ(10u, io.mask());
    EXPECT_EQ(ReceiveResult::Applied, feed(app, "M1\n"));
    EXPECT_EQ(0u, io.mask());
}

TEST_F(ControllerApplicationTest, DiscardsCorruptedCommandsAtEveryBytePositionAndRecovers) {
    for (const std::string line : {"M2\n", "T5\r\n", "0101\n", "LISTA\n"}) {
        for (size_t bad = 0; bad < line.size(); ++bad) {
            app_.initialize();
            ASSERT_EQ(ReceiveResult::Applied, feed(app_, "M4\n1010\n"));
            io_.clear();
            EXPECT_EQ(ReceiveResult::Pending, feed(app_, line.substr(0, bad)));
            app_.discard_line();
            const std::string tail = line[bad] == '\n' ? "T2\n" : line.substr(bad + 1);
            EXPECT_TRUE(io_.order.empty());
            expect_rejected(tail);
            EXPECT_EQ(kInvalidLine, io_.reply);
            EXPECT_EQ(4, app_.state().mode());
            EXPECT_EQ(5u, io_.mask());
            expect_change("0101\n", 4, 10, "OK: saidas atualizadas.\n");
        }
    }
}

TEST_F(ControllerApplicationTest, RepeatedErrorsProduceOneRejectionAtTheNextValidLf) {
    ASSERT_EQ(ReceiveResult::Pending, feed(app_, "M4\n1010\n10"));
    io_.clear();
    for (int error = 0; error < 3; ++error) {
        app_.discard_line();
        EXPECT_EQ(ReceiveResult::Pending, feed(app_, "T2\r"));
        EXPECT_TRUE(io_.order.empty());
        EXPECT_EQ(5u, io_.mask());
    }
    expect_rejected("\n");
    EXPECT_EQ(kInvalidLine, io_.reply);
    io_.clear();
    EXPECT_EQ(ReceiveResult::Pending, feed(app_, "\n"));
    EXPECT_TRUE(io_.reply.empty());
    expect_change("T5\n", 4, 3, "OK: tabela aplicada.\n");
}

TEST_F(ControllerApplicationTest, RejectsMalformedLinesWithoutTouchingPinsAndRecovers) {
    ASSERT_EQ(ReceiveResult::Applied, feed(app_, "M4\n0101\n"));
    for (const std::string& line : {std::string(33, '1') + "M1\n",
                                   std::string("M2\r\r\n"), std::string("M\r2\n")}) {
        expect_rejected(line);
        EXPECT_EQ(kInvalidLine, io_.reply);
    }
    expect_change("T5\n", 4, 3, "OK: tabela aplicada.\n");
}

TEST_F(ControllerApplicationTest, PreservesReplyAndPinOrderAcrossSuccessesListsAndErrors) {
    EXPECT_EQ(ReceiveResult::Rejected, feed(app_, "M1\nT2\nLISTA\n0\nT3\n"));
    EXPECT_EQ("OK: modo M1; saidas desligadas.\nOK: tabela aplicada.\n"
              "M1\nT1: 0\nT2: 1\nOK: saidas atualizadas.\n"
              "ERRO: tabela invalida; use T1 a T2.\n", io_.reply);
    EXPECT_EQ(96, std::count(io_.order.begin(), io_.order.end(), 'W'));
    EXPECT_EQ(std::string::npos, io_.order.find('C'));
    EXPECT_EQ(0u, io_.mask());
    EXPECT_EQ(0u, app_.state().outputs());
}

}
