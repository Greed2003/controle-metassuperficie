#include "metasurface/avr_digital_output.hpp"
#include "metasurface/controller_state.hpp"
#include "metasurface/output_control.hpp"
#include "metasurface/command_processor.hpp"

#include <gtest/gtest.h>
#include <array>

namespace {

using Registers = std::array<uint8_t, 12>;

metasurface::AvrGpioRegisters bind(Registers& r) {
    return {{r[0], r[1]}, {r[2], r[3]}, {r[4], r[5]},
            {r[6], r[7]}, {r[8], r[9]}, {r[10], r[11]}};
}

struct Mapping { size_t port; uint8_t mask; };

// D22-D53, from the Mega 2560 pinout. Port order: A, B, C, D, G, L.
const Mapping kPins[] = {
    {0, 0x01}, {0, 0x02}, {0, 0x04}, {0, 0x08},
    {0, 0x10}, {0, 0x20}, {0, 0x40}, {0, 0x80},
    {2, 0x80}, {2, 0x40}, {2, 0x20}, {2, 0x10},
    {2, 0x08}, {2, 0x04}, {2, 0x02}, {2, 0x01},
    {3, 0x80}, {4, 0x04}, {4, 0x02}, {4, 0x01},
    {5, 0x80}, {5, 0x40}, {5, 0x20}, {5, 0x10},
    {5, 0x08}, {5, 0x04}, {5, 0x02}, {5, 0x01},
    {1, 0x08}, {1, 0x04}, {1, 0x02}, {1, 0x01}
};

enum class Operation { High, Low, Configure };

class AvrDigitalOutputTest : public testing::Test {
protected:
    AvrDigitalOutputTest() : output_(bind(registers_)) {}

    void check_bit_updates(Operation operation) {
        for (size_t channel = 0; channel < 32; ++channel) {
            SCOPED_TRACE(channel);
            const Mapping pin = kPins[channel];
            for (int initial = 0; initial < 256; ++initial) {
                SCOPED_TRACE(initial);
                registers_.fill(static_cast<uint8_t>(initial));
                Registers expected = registers_;
                const bool configure = operation == Operation::Configure;
                const size_t index = 2 * pin.port + (configure ? 0 : 1);
                if (operation == Operation::Low) {
                    expected[index] &= static_cast<uint8_t>(~pin.mask);
                } else {
                    expected[index] |= pin.mask;
                }
                const uint8_t number = static_cast<uint8_t>(22 + channel);
                if (configure) {
                    output_.configure_output(number);
                } else {
                    output_.write(number, operation == Operation::High);
                }
                ASSERT_EQ(expected, registers_);
            }
        }
    }

    void check_apply(uint32_t bits) {
        ASSERT_TRUE(state_.set_outputs(bits));
        Registers expected = registers_;
        for (size_t channel = 0; channel < 32; ++channel) {
            const Mapping pin = kPins[channel];
            uint8_t& value = expected[2 * pin.port + 1];
            value &= static_cast<uint8_t>(~pin.mask);
            if ((bits & (uint32_t(1) << channel)) != 0) {
                value |= pin.mask;
            }
        }
        metasurface::apply_outputs(state_, output_);
        EXPECT_EQ(expected, registers_);
    }

    Registers registers_{};
    metasurface::AvrDigitalOutput output_;
    metasurface::ControllerState state_;
};

TEST_F(AvrDigitalOutputTest, ConstructionDoesNotChangeRegisters) {
    registers_ = {{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}};
    const Registers expected = registers_;
    metasurface::AvrDigitalOutput another(bind(registers_));
    EXPECT_EQ(expected, registers_);
}

TEST_F(AvrDigitalOutputTest, HighChangesOnlyTheMappedPortBit) {
    check_bit_updates(Operation::High);
}

TEST_F(AvrDigitalOutputTest, LowChangesOnlyTheMappedPortBit) {
    check_bit_updates(Operation::Low);
}

TEST_F(AvrDigitalOutputTest, ConfigureChangesOnlyTheMappedDirectionBit) {
    check_bit_updates(Operation::Configure);
}

TEST_F(AvrDigitalOutputTest, ConfigurationPreservesThePreviouslyWrittenLevel) {
    output_.write(22, true);
    EXPECT_EQ(0, registers_[0]);
    EXPECT_EQ(1, registers_[1]);
    output_.configure_output(22);
    EXPECT_EQ(1, registers_[0]);
    EXPECT_EQ(1, registers_[1]);
    output_.write(22, false);
    EXPECT_EQ(1, registers_[0]);
    EXPECT_EQ(0, registers_[1]);
}

TEST_F(AvrDigitalOutputTest, UnmappedPinsLeaveAllRegistersUnchanged) {
    registers_ = {{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}};
    const Registers expected = registers_;
    for (int pin = 0; pin < 256; ++pin) {
        if (pin >= 22 && pin <= 53) { continue; }
        const uint8_t number = static_cast<uint8_t>(pin);
        output_.write(number, true);
        EXPECT_EQ(expected, registers_) << pin;
        output_.write(number, false);
        EXPECT_EQ(expected, registers_) << pin;
        output_.configure_output(number);
        EXPECT_EQ(expected, registers_) << pin;
    }
}

TEST_F(AvrDigitalOutputTest, InitializationClearsAndEnablesOnlyAssignedPins) {
    registers_ = {{0x55, 0xff, 0x55, 0xff, 0x55, 0xff,
                   0x55, 0xff, 0x55, 0xff, 0x55, 0xff}};
    metasurface::initialize_outputs(output_);
    const Registers expected = {{0xff, 0x00, 0x5f, 0xf0, 0xff, 0x00,
                                 0xd5, 0x7f, 0x57, 0xf8, 0xff, 0x00}};
    EXPECT_EQ(expected, registers_);
}

TEST_F(AvrDigitalOutputTest, InitialStateClearsLevelsWithoutConfiguringPins) {
    registers_.fill(0xff);
    metasurface::apply_outputs(state_, output_);
    const Registers expected = {{0xff, 0x00, 0xff, 0xf0, 0xff, 0x00,
                                 0xff, 0x7f, 0xff, 0xf8, 0xff, 0x00}};
    EXPECT_EQ(expected, registers_);
}

TEST_F(AvrDigitalOutputTest, ReplacesHighAndLowLevelsPreservingOtherPins) {
    registers_.fill(0xa5);
    ASSERT_TRUE(state_.select_mode(32));
    check_apply(UINT32_MAX);
    check_apply(0);
    check_apply(UINT32_MAX);
}

TEST_F(AvrDigitalOutputTest, AppliesEachChannelInEveryMode) {
    registers_.fill(0x5a);
    for (int mode = 1; mode <= 32; ++mode) {
        SCOPED_TRACE(mode);
        ASSERT_TRUE(state_.select_mode(mode));
        check_apply(UINT32_MAX >> (32 - mode));
        for (int channel = 0; channel < mode; ++channel) {
            check_apply(uint32_t(1) << channel);
        }
    }
}

TEST_F(AvrDigitalOutputTest, TableCommandsProduceExpectedPortValues) {
    ASSERT_TRUE(metasurface::process_command("M32", 3, state_));
    ASSERT_TRUE(metasurface::process_command("T9", 2, state_));
    metasurface::apply_outputs(state_, output_);
    const Registers ends = {{0, 0x01, 0, 0x01, 0, 0, 0, 0, 0, 0, 0, 0}};
    EXPECT_EQ(ends, registers_);
    ASSERT_TRUE(metasurface::process_command("T12", 3, state_));
    metasurface::apply_outputs(state_, output_);
    const Registers upper = {{0, 0, 0, 0x0f, 0, 0, 0, 0x80, 0, 0x07, 0, 0xff}};
    EXPECT_EQ(upper, registers_);
}

}
