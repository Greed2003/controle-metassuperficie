#include "metasurface/serial_transport.hpp"
#include "metasurface/avr_uart.hpp"
#include "metasurface/digital_output.hpp"
#include "metasurface/pattern_catalog.hpp"

#include <gtest/gtest.h>
#include <deque>
#include <string>
#include <vector>

namespace {

using metasurface::UartReadResult;
const char* const kInvalidLine =
    "ERRO: linha invalida; use ate 32 caracteres e termine com LF ou CRLF.\n";

class FakeIo : public metasurface::Uart, public metasurface::DigitalOutput {
public:
    struct Input { uint8_t value; UartReadResult result; };
    std::deque<Input> input;
    std::string output, order;
    std::vector<uint32_t> levels_at_tx;
    uint32_t levels = UINT32_MAX;
    bool ready = true;
    size_t reads = 0, writes = 0, stride = 1;

    void initialize() override { input.clear(); order += 'I'; }
    UartReadResult try_read(uint8_t& value) override {
        ++reads;
        if (input.empty()) return UartReadResult::Empty;
        const Input next = input.front();
        input.pop_front();
        if (next.result == UartReadResult::Received) value = next.value;
        return next.result;
    }
    bool try_write(uint8_t value) override {
        ++writes;
        if (!ready || writes % stride != 0) return false;
        output += static_cast<char>(value);
        levels_at_tx.push_back(levels);
        order += 'T';
        return true;
    }
    void write(uint8_t pin, bool high) override {
        ASSERT_GE(pin, 22);
        ASSERT_LE(pin, 53);
        const uint32_t bit = uint32_t(1) << (pin - 22);
        levels = high ? levels | bit : levels & ~bit;
        order += 'W';
    }
    void configure_output(uint8_t) override {
        EXPECT_EQ(0u, levels);
        order += 'C';
    }
    void queue(const std::string& text) {
        for (unsigned char c : text) input.push_back(Input{c, UartReadResult::Received});
    }
    void clear() { output.clear(); order.clear(); levels_at_tx.clear(); reads = writes = 0; }
};

class SerialTransportTest : public testing::Test {
protected:
    SerialTransportTest() : transport_(io_, io_) {}
    void SetUp() override { transport_.initialize(); io_.clear(); }
    void poll(size_t count = 2048) {
        for (size_t i = 0; i < count; ++i) {
            const size_t reads = io_.reads, writes = io_.writes;
            transport_.poll();
            EXPECT_LE(io_.reads - reads, 1u);
            EXPECT_LE(io_.writes - writes, 1u);
        }
        EXPECT_FALSE(transport_.faulted());
    }
    void feed(const std::string& text) { io_.queue(text); poll(text.size()); }
    void request(const std::string& text) {
        feed(text);
        poll();
        EXPECT_FALSE(transport_.response_pending());
    }
    void expect_levels(uint32_t levels) {
        EXPECT_EQ(levels, transport_.state().outputs());
        EXPECT_EQ(levels, io_.levels);
        for (uint32_t observed : io_.levels_at_tx) EXPECT_EQ(levels, observed);
    }
    FakeIo io_;
    metasurface::SerialTransport transport_;
};

TEST_F(SerialTransportTest, ConstructionAndPollingBeforeInitializationHaveNoEffects) {
    FakeIo io;
    metasurface::SerialTransport transport(io, io);
    io.queue("M1\n");
    for (int i = 0; i < 20; ++i) transport.poll();
    EXPECT_TRUE(io.order.empty());
    EXPECT_EQ(0u, io.reads);
    EXPECT_EQ(0u, io.writes);
    EXPECT_EQ(UINT32_MAX, io.levels);
    EXPECT_EQ(0, transport.state().mode());
    EXPECT_FALSE(transport.response_pending());
    EXPECT_FALSE(transport.faulted());
}

TEST_F(SerialTransportTest, InitializesAllPinsLowBeforeEnablingUart) {
    transport_.initialize();
    EXPECT_EQ(std::string(32, 'W') + std::string(32, 'C') + "I", io_.order);
    EXPECT_EQ(0u, io_.levels);
    EXPECT_EQ(0, transport_.state().mode());
    EXPECT_TRUE(io_.output.empty());
}

TEST_F(SerialTransportTest, WaitsForLfAndAcceptsFragmentedCrLfAndEmptyLines) {
    request("\n\r\n");
    EXPECT_TRUE(io_.output.empty());
    feed("M32\r");
    poll();
    EXPECT_TRUE(io_.output.empty());
    EXPECT_EQ(0, transport_.state().mode());
    request("\n");
    EXPECT_EQ("OK: modo M32; saidas desligadas.\n", io_.output);
}

TEST_F(SerialTransportTest, RetriesBusyTransmitterWithoutLosingOrDuplicatingBytes) {
    io_.ready = false;
    feed("M4\nT5\n");
    poll();
    EXPECT_TRUE(io_.output.empty());
    EXPECT_TRUE(io_.input.empty());
    EXPECT_TRUE(transport_.response_pending());
    EXPECT_EQ(4, transport_.state().mode());
    EXPECT_EQ(0u, io_.levels);
    io_.ready = true;
    io_.stride = 3;
    poll();
    EXPECT_EQ("OK: modo M4; saidas desligadas.\nOK: tabela aplicada.\n", io_.output);
    EXPECT_EQ(3u, io_.levels);
    EXPECT_FALSE(transport_.response_pending());
}

TEST_F(SerialTransportTest, AppliesModesAllTablesAndBinaryCommandsBeforeTransmitting) {
    for (int mode = 1; mode <= 32; ++mode) {
        io_.clear();
        request("M" + std::to_string(mode) + "\n");
        EXPECT_EQ("OK: modo M" + std::to_string(mode) + "; saidas desligadas.\n", io_.output);
        expect_levels(0);
        EXPECT_EQ(32u, io_.order.find('T'));
        for (int table = 1; table <= metasurface::pattern_count(mode); ++table) {
            uint32_t expected = 0;
            ASSERT_TRUE(metasurface::get_pattern(mode, table, expected));
            io_.clear();
            request("T" + std::to_string(table) + "\n");
            EXPECT_EQ("OK: tabela aplicada.\n", io_.output);
            expect_levels(expected);
            EXPECT_EQ(32u, io_.order.find('T'));
        }
        io_.clear();
        request(std::string(mode, '1') + "\n");
        expect_levels(mode == 32 ? UINT32_MAX : (uint32_t(1) << mode) - 1);
        EXPECT_EQ("OK: saidas atualizadas.\n", io_.output);
    }
}

TEST_F(SerialTransportTest, TransmitsEveryCompleteListWithPausesIncluding451ByteResponse) {
    io_.stride = 3;
    for (int mode = 1; mode <= 32; ++mode) {
        request("M" + std::to_string(mode) + "\nT2\n");
        const uint32_t levels = io_.levels;
        io_.clear();
        std::string expected = "M" + std::to_string(mode) + "\n";
        for (int table = 1; table <= metasurface::pattern_count(mode); ++table) {
            uint32_t mask = 0;
            ASSERT_TRUE(metasurface::get_pattern(mode, table, mask));
            expected += "T" + std::to_string(table) + ": ";
            for (int bit = 0; bit < mode; ++bit) expected += (mask & (uint32_t(1) << bit)) ? '1' : '0';
            expected += '\n';
        }
        request("LISTA\n");
        EXPECT_EQ(expected, io_.output);
        EXPECT_EQ(std::string::npos, io_.order.find_first_of("WC"));
        expect_levels(levels);
        EXPECT_LE(expected.size(), metasurface::kSerialResponseCapacity);
        if (mode == 32) EXPECT_EQ(451u, expected.size());
    }
}

TEST_F(SerialTransportTest, PreservesResponseOrderAcrossQueuedCommandsAndLists) {
    request("M1\nT2\nLISTA\n0\nT3\n");
    EXPECT_EQ("OK: modo M1; saidas desligadas.\nOK: tabela aplicada.\n"
              "M1\nT1: 0\nT2: 1\nOK: saidas atualizadas.\n"
              "ERRO: tabela invalida; use T1 a T2.\n", io_.output);
    EXPECT_EQ(0u, io_.levels);
}

TEST_F(SerialTransportTest, RejectsInvalidCommandsInEveryModeWithoutChangingPins) {
    request("LISTA\n");
    EXPECT_EQ("ERRO: selecione um modo de M1 a M32.\n", io_.output);
    for (int mode = 1; mode <= 32; ++mode) {
        request("M" + std::to_string(mode) + "\nT2\n");
        const uint32_t levels = io_.levels;
        for (const std::string& command : {std::string("M0\n"), std::string("T99\n"),
             std::string(33, '1') + "\n", std::string(mode, '0') + "0\n",
             std::string("X\n"), std::string("M\r2\n")}) {
            io_.clear();
            request(command);
            EXPECT_EQ(0u, io_.output.find("ERRO:"));
            EXPECT_EQ(std::string::npos, io_.order.find_first_of("WC"));
            EXPECT_EQ(mode, transport_.state().mode());
            expect_levels(levels);
        }
    }
}

TEST_F(SerialTransportTest, EmbeddedNullIsNotTreatedAsEndOfCommand) {
    request("M4\nT2\n");
    io_.clear();
    request(std::string("T5\0\n", 4));
    EXPECT_EQ("ERRO: tabela invalida; use T1 a T12.\n", io_.output);
    expect_levels(15);
}

TEST_F(SerialTransportTest, UartErrorAtEveryPositionDiscardsWholeLineAndRecovers) {
    for (const std::string& command : {std::string("M2\n"), std::string("T5\r\n"),
                                       std::string("0101\n"), std::string("LISTA\n")}) {
        for (size_t bad = 0; bad < command.size(); ++bad) {
            request("M4\nT2\n");
            io_.clear();
            for (size_t i = 0; i < command.size(); ++i) {
                io_.input.push_back(FakeIo::Input{static_cast<uint8_t>(command[i]),
                    i == bad ? UartReadResult::Error : UartReadResult::Received});
                poll(1);
            }
            if (bad == command.size() - 1) feed("T5\n");
            poll();
            EXPECT_EQ(kInvalidLine, io_.output);
            EXPECT_EQ(4, transport_.state().mode());
            expect_levels(15);
            EXPECT_EQ(std::string::npos, io_.order.find_first_of("WC"));
            io_.clear();
            request("T5\n");
            EXPECT_EQ("OK: tabela aplicada.\n", io_.output);
            expect_levels(3);
        }
    }
}

TEST_F(SerialTransportTest, QueuesErrorsDuringTransmissionAndRejectsOnceAtCleanLf) {
    request("M4\n");
    io_.clear();
    io_.ready = false;
    feed("T2\nT");
    for (int i = 0; i < 5; ++i) {
        io_.input.push_back(FakeIo::Input{10, UartReadResult::Error});
        poll(1);
    }
    feed("5\nT5\n");
    EXPECT_TRUE(io_.input.empty());
    io_.ready = true;
    poll();
    EXPECT_EQ(std::string("OK: tabela aplicada.\n") + kInvalidLine + "OK: tabela aplicada.\n", io_.output);
    EXPECT_EQ(3u, io_.levels);
}

TEST_F(SerialTransportTest, FullReceiveQueueRetainsAllBytesWhenNoMoreInputArrives) {
    io_.ready = false;
    feed("M1\n");
    std::string commands;
    for (int i = 0; i < 32; ++i) commands += "1\n";
    feed(commands);
    EXPECT_EQ(64u, commands.size());
    io_.ready = true;
    poll();
    std::string expected = "OK: modo M1; saidas desligadas.\n";
    for (int i = 0; i < 32; ++i) expected += "OK: saidas atualizadas.\n";
    EXPECT_EQ(expected, io_.output);
    EXPECT_EQ(1u, io_.levels);
}

TEST_F(SerialTransportTest, OverflowDiscardsUnreadCommandsAndResynchronizesAtCleanLf) {
    io_.ready = false;
    feed("M4\n");
    std::string queued;
    for (int i = 0; i < 21; ++i) queued += "T2\n";
    feed(queued + "T");
    feed("5\n");
    io_.ready = true;
    poll();
    EXPECT_EQ(std::string("OK: modo M4; saidas desligadas.\n") + kInvalidLine, io_.output);
    EXPECT_EQ(0u, io_.levels);
    io_.clear();
    request("T5\n");
    expect_levels(3);
}

TEST_F(SerialTransportTest, ValidLfThatOverflowsQueueCanTerminateDiscardedLine) {
    io_.ready = false;
    feed("M1\n");
    feed(std::string(64, '1') + "\n");
    io_.ready = true;
    poll();
    EXPECT_EQ(std::string("OK: modo M1; saidas desligadas.\n") + kInvalidLine, io_.output);
    EXPECT_EQ(0u, io_.levels);
}

TEST_F(SerialTransportTest, RepeatedOverflowsAndFaultyLfDoNotAcceptCommandSuffix) {
    io_.ready = false;
    feed("M4\n");
    feed(std::string(256, '1'));
    io_.input.push_back(FakeIo::Input{10, UartReadResult::Error});
    poll(1);
    feed("T2\n");
    io_.ready = true;
    poll();
    EXPECT_EQ(std::string("OK: modo M4; saidas desligadas.\n") + kInvalidLine, io_.output);
    EXPECT_EQ(0u, io_.levels);
    io_.clear();
    request("T5\n");
    expect_levels(3);
}

TEST_F(SerialTransportTest, ReinitializationDropsPendingInputAndSoftwareResponse) {
    feed("M4\n");
    ASSERT_EQ("O", io_.output);
    io_.ready = false;
    feed("T2\nM");
    io_.clear();
    transport_.initialize();
    EXPECT_EQ(std::string(32, 'W') + std::string(32, 'C') + "I", io_.order);
    EXPECT_FALSE(transport_.response_pending());
    EXPECT_EQ(0, transport_.state().mode());
    io_.ready = true;
    io_.clear();
    request("\n");
    EXPECT_TRUE(io_.output.empty());
    request("M1\n");
    EXPECT_EQ("OK: modo M1; saidas desligadas.\n", io_.output);
}

TEST_F(SerialTransportTest, ReinitializationClearsPartialLineAndErrorDiscard) {
    for (bool error : {false, true}) {
        feed("M");
        if (error) {
            io_.input.push_back(FakeIo::Input{0, UartReadResult::Error});
            poll(1);
        }
        transport_.initialize();
        io_.clear();
        request("M2\n");
        EXPECT_EQ("OK: modo M2; saidas desligadas.\n", io_.output);
        EXPECT_EQ(2, transport_.state().mode());
    }
}

TEST_F(SerialTransportTest, AvrUartWorksThroughTheTransportInterface) {
    uint8_t power = 0xff, status = 0, control_b = 0, control_c = 0;
    uint8_t baud_high = 0, baud_low = 0, data = 0;
    metasurface::AvrUart uart({power, status, control_b, control_c, baud_high, baud_low, data});
    FakeIo pins;
    metasurface::SerialTransport transport(uart, pins);
    transport.initialize();
    EXPECT_EQ(0xfdu, power);
    EXPECT_EQ(0x18u, control_b);
    for (char c : std::string("M1\n")) {
        data = static_cast<uint8_t>(c);
        status = 0x82;
        transport.poll();
    }
    EXPECT_EQ(1, transport.state().mode());
    std::string output;
    for (int i = 0; i < 512 && transport.response_pending(); ++i) {
        status = 0x22;
        transport.poll();
        output += static_cast<char>(data);
    }
    EXPECT_EQ("OK: modo M1; saidas desligadas.\n", output);
    EXPECT_FALSE(transport.response_pending());
}

}
