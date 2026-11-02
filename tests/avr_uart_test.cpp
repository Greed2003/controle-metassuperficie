#include "metasurface/avr_uart.hpp"

#include <gtest/gtest.h>
#include <array>

namespace {

using Registers = std::array<uint8_t, 7>;
using Result = metasurface::UartReadResult;
enum Index { Power, Status, ControlB, ControlC, BaudHigh, BaudLow, Data };

metasurface::AvrUartRegisters bind(Registers& r) {
    return {r[Power], r[Status], r[ControlB], r[ControlC],
            r[BaudHigh], r[BaudLow], r[Data]};
}

class AvrUartTest : public testing::Test {
protected:
    AvrUartTest() : uart_(bind(registers_)) {}
    Registers registers_{};
    metasurface::AvrUart uart_;
};

TEST_F(AvrUartTest, ConstructionDoesNotChangeRegisters) {
    registers_ = {{1, 2, 3, 4, 5, 6, 7}};
    const Registers expected = registers_;
    metasurface::AvrUart another(bind(registers_));
    EXPECT_EQ(expected, registers_);
}

TEST_F(AvrUartTest, Initializes115200EightNOneWithoutUartInterrupts) {
    registers_.fill(0xff);
    uart_.initialize();
    const Registers expected = {{0xfd, 0x02, 0x18, 0x06, 0x00, 0x10, 0xff}};
    EXPECT_EQ(expected, registers_);
}

TEST_F(AvrUartTest, InitializationPreservesOtherPowerBits) {
    for (int initial = 0; initial < 256; ++initial) {
        registers_[Power] = static_cast<uint8_t>(initial);
        uart_.initialize();
        EXPECT_EQ(initial & 0xfd, registers_[Power]) << initial;
    }
}

TEST_F(AvrUartTest, EmptyReceivePreservesDestinationAndRegisters) {
    uart_.initialize();
    registers_[Data] = 0xa5;
    for (int status = 0; status < 128; ++status) {
        registers_[Status] = static_cast<uint8_t>(status);
        const Registers expected = registers_;
        uint8_t value = 0x5a;
        EXPECT_EQ(Result::Empty, uart_.try_read(value)) << status;
        EXPECT_EQ(0x5a, value);
        EXPECT_EQ(expected, registers_);
    }
}

TEST_F(AvrUartTest, ReceivesAllByteValuesIncludingNull) {
    uart_.initialize();
    registers_[Status] = 0xe2;
    for (int byte = 0; byte < 256; ++byte) {
        registers_[Data] = static_cast<uint8_t>(byte);
        const Registers expected = registers_;
        uint8_t value = static_cast<uint8_t>(~byte);
        ASSERT_EQ(Result::Received, uart_.try_read(value));
        EXPECT_EQ(byte, value);
        EXPECT_EQ(expected, registers_);
    }
}

TEST_F(AvrUartTest, RejectsEveryReceiveErrorCombination) {
    uart_.initialize();
    const uint8_t errors[] = {0x04, 0x08, 0x0c, 0x10, 0x14, 0x18, 0x1c};
    for (uint8_t error : errors) {
        registers_[Status] = static_cast<uint8_t>(0x80 | error);
        registers_[Data] = 0xa5;
        const Registers expected = registers_;
        uint8_t value = 0x5a;
        EXPECT_EQ(Result::Error, uart_.try_read(value)) << int(error);
        EXPECT_EQ(0x5a, value);
        EXPECT_EQ(expected, registers_);
    }
}

TEST_F(AvrUartTest, RecoversWhenTheNextReceivedByteHasNoError) {
    uart_.initialize();
    registers_[Status] = 0x9c;
    registers_[Data] = 0xff;
    uint8_t value = 0;
    EXPECT_EQ(Result::Error, uart_.try_read(value));
    // Simulate the hardware presenting the next FIFO entry.
    registers_[Status] = 0x82;
    registers_[Data] = 0x4d;
    EXPECT_EQ(Result::Received, uart_.try_read(value));
    EXPECT_EQ(0x4d, value);
}

TEST_F(AvrUartTest, DisabledReceiverOrClockPreservesDestination) {
    for (int disabled = 0; disabled < 2; ++disabled) {
        uart_.initialize();
        registers_[Status] = 0x82;
        registers_[Data] = 0xa5;
        if (disabled == 0) { registers_[ControlB] = 0x08; }
        else { registers_[Power] = 0x02; }
        const Registers expected = registers_;
        uint8_t value = 0x5a;
        EXPECT_EQ(Result::Empty, uart_.try_read(value));
        EXPECT_EQ(0x5a, value);
        EXPECT_EQ(expected, registers_);
    }
}

TEST_F(AvrUartTest, WritesOnlyWhenTransmitBufferIsReady) {
    uart_.initialize();
    for (int status = 0; status < 256; ++status) {
        registers_[Status] = static_cast<uint8_t>(status);
        registers_[Data] = 0xa5;
        Registers expected = registers_;
        const bool ready = (status & 0x20) != 0;
        if (ready) { expected[Data] = 0x5a; }
        EXPECT_EQ(ready, uart_.try_write(0x5a)) << status;
        EXPECT_EQ(expected, registers_);
    }
}

TEST_F(AvrUartTest, TransmitsAllByteValuesIncludingNull) {
    uart_.initialize();
    for (int byte = 0; byte < 256; ++byte) {
        registers_[Status] = 0x22;
        registers_[Data] = static_cast<uint8_t>(~byte);
        Registers expected = registers_;
        expected[Data] = static_cast<uint8_t>(byte);
        ASSERT_TRUE(uart_.try_write(static_cast<uint8_t>(byte)));
        EXPECT_EQ(expected, registers_);
    }
}

TEST_F(AvrUartTest, DisabledTransmitterOrClockPreservesRegisters) {
    for (int disabled = 0; disabled < 2; ++disabled) {
        uart_.initialize();
        registers_[Status] = 0x22;
        registers_[Data] = 0xa5;
        if (disabled == 0) { registers_[ControlB] = 0x10; }
        else { registers_[Power] = 0x02; }
        const Registers expected = registers_;
        EXPECT_FALSE(uart_.try_write(0x5a));
        EXPECT_EQ(expected, registers_);
    }
}

}
