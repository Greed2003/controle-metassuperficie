#include "metasurface/output_control.hpp"
#include "metasurface/controller_state.hpp"
#include "metasurface/digital_output.hpp"
#include "metasurface/command_processor.hpp"
#include "metasurface/text_output.hpp"

#include <gtest/gtest.h>
#include <array>
#include <cstring>
#include <string>
#include <vector>

namespace {

enum class Operation { Write, Configure };

struct PinEvent {
    Operation operation;
    uint8_t pin;
    bool high;
};

class RecordingPins : public metasurface::DigitalOutput {
public:
    RecordingPins() { levels.fill(true); }

    void write(uint8_t pin, bool high) override {
        ASSERT_GE(pin, 22);
        ASSERT_LE(pin, 53);
        levels[pin] = high;
        events.push_back({Operation::Write, pin, high});
    }

    void configure_output(uint8_t pin) override {
        ASSERT_GE(pin, 22);
        ASSERT_LE(pin, 53);
        events.push_back({Operation::Configure, pin, levels[pin]});
    }

    std::array<bool, 54> levels;
    std::vector<PinEvent> events;
};

class ListOutput : public metasurface::TextOutput {
public:
    void write(const char* text, size_t length) override { value.append(text, length); }
    std::string value;
};

class OutputControlTest : public testing::Test {
protected:
    void expect_levels(std::string bits) {
        ASSERT_LE(bits.size(), 32u);
        bits.resize(32, '0');
        for (size_t channel = 0; channel < bits.size(); ++channel) {
            EXPECT_EQ(bits[channel] == '1', pins_.levels[22 + channel]) << channel + 1;
        }
        for (int pin = 0; pin < 22; ++pin) {
            EXPECT_TRUE(pins_.levels[pin]);
        }
    }

    metasurface::ControllerState state_;
    RecordingPins pins_;
};

TEST_F(OutputControlTest, WritesLowBeforeConfiguringAnyPin) {
    metasurface::initialize_outputs(pins_);
    ASSERT_EQ(64u, pins_.events.size());
    for (size_t channel = 0; channel < 32; ++channel) {
        const PinEvent& write = pins_.events[channel];
        EXPECT_EQ(Operation::Write, write.operation);
        EXPECT_EQ(22 + channel, write.pin);
        EXPECT_FALSE(write.high);
        const PinEvent& configure = pins_.events[32 + channel];
        EXPECT_EQ(Operation::Configure, configure.operation);
        EXPECT_EQ(22 + channel, configure.pin);
        EXPECT_FALSE(configure.high);
    }
    expect_levels("");
}

TEST_F(OutputControlTest, ReinitializationTurnsEveryOutputOff) {
    metasurface::initialize_outputs(pins_);
    ASSERT_TRUE(state_.select_mode(32));
    ASSERT_TRUE(state_.set_outputs(UINT32_MAX));
    metasurface::apply_outputs(state_, pins_);
    expect_levels(std::string(32, '1'));
    metasurface::initialize_outputs(pins_);
    expect_levels("");
}

TEST_F(OutputControlTest, AppliesInitialStateAsAllLow) {
    metasurface::apply_outputs(state_, pins_);
    expect_levels("");
    EXPECT_EQ(0, state_.mode());
}

TEST_F(OutputControlTest, AppliesAllThirtyTwoHighOutputs) {
    ASSERT_TRUE(state_.select_mode(32));
    ASSERT_TRUE(state_.set_outputs(UINT32_MAX));
    metasurface::apply_outputs(state_, pins_);
    expect_levels(std::string(32, '1'));
}

TEST_F(OutputControlTest, MapsEachChannelToItsOwnPin) {
    ASSERT_TRUE(state_.select_mode(32));
    for (int channel = 0; channel < 32; ++channel) {
        ASSERT_TRUE(state_.set_outputs(uint32_t(1) << channel));
        metasurface::apply_outputs(state_, pins_);
        std::string expected(32, '0');
        expected[channel] = '1';
        expect_levels(expected);
    }
}

TEST_F(OutputControlTest, ReplacesAllLevelsWithSuccessivePatterns) {
    ASSERT_TRUE(state_.select_mode(32));
    ASSERT_TRUE(state_.set_outputs(0x55555555u));
    metasurface::apply_outputs(state_, pins_);
    expect_levels("10101010101010101010101010101010");
    ASSERT_TRUE(state_.set_outputs(0xaaaaaaaau));
    metasurface::apply_outputs(state_, pins_);
    expect_levels("01010101010101010101010101010101");
    ASSERT_TRUE(state_.set_outputs(0));
    metasurface::apply_outputs(state_, pins_);
    expect_levels("");
}

TEST_F(OutputControlTest, KeepsOutputsOutsideEveryModeLow) {
    for (int mode = 1; mode <= 32; ++mode) {
        ASSERT_TRUE(state_.select_mode(32));
        ASSERT_TRUE(state_.set_outputs(UINT32_MAX));
        metasurface::apply_outputs(state_, pins_);
        ASSERT_TRUE(state_.select_mode(mode));
        ASSERT_TRUE(state_.set_outputs(UINT32_MAX >> (32 - mode)));
        metasurface::apply_outputs(state_, pins_);
        expect_levels(std::string(mode, '1'));
    }
}

TEST_F(OutputControlTest, ModeSelectionAndReselectionClearAllPins) {
    ASSERT_TRUE(state_.select_mode(32));
    for (const char* command : {"M4", "M4", "M1", "M32"}) {
        ASSERT_TRUE(state_.set_outputs(1));
        metasurface::apply_outputs(state_, pins_);
        ASSERT_TRUE(metasurface::process_command(command, std::strlen(command), state_));
        metasurface::apply_outputs(state_, pins_);
        expect_levels("");
    }
}

TEST_F(OutputControlTest, AppliesBinaryCommandsInChannelOrder) {
    ASSERT_TRUE(metasurface::process_command("M4", 2, state_));
    ASSERT_TRUE(metasurface::process_command("0101", 4, state_));
    metasurface::apply_outputs(state_, pins_);
    expect_levels("0101");
}

TEST_F(OutputControlTest, AppliesTablesInChannelOrder) {
    ASSERT_TRUE(metasurface::process_command("M32", 3, state_));
    ASSERT_TRUE(metasurface::process_command("T9", 2, state_));
    metasurface::apply_outputs(state_, pins_);
    expect_levels("10000000000000000000000000000001");
    ASSERT_TRUE(metasurface::process_command("M4", 2, state_));
    ASSERT_TRUE(metasurface::process_command("T5", 2, state_));
    metasurface::apply_outputs(state_, pins_);
    expect_levels("1100");
}

TEST_F(OutputControlTest, ApplyingOutputsPreservesLogicalState) {
    ASSERT_TRUE(state_.select_mode(32));
    ASSERT_TRUE(state_.set_outputs(0x80000001u));
    metasurface::apply_outputs(state_, pins_);
    metasurface::apply_outputs(state_, pins_);
    EXPECT_EQ(32, state_.mode());
    EXPECT_EQ(0x80000001u, state_.outputs());
    expect_levels("10000000000000000000000000000001");
}

TEST_F(OutputControlTest, ApplyingOutputsDoesNotReconfigurePins) {
    metasurface::initialize_outputs(pins_);
    pins_.events.clear();
    ASSERT_TRUE(state_.select_mode(1));
    ASSERT_TRUE(state_.set_outputs(1));
    metasurface::apply_outputs(state_, pins_);
    ASSERT_EQ(32u, pins_.events.size());
    for (const PinEvent& event : pins_.events) {
        EXPECT_EQ(Operation::Write, event.operation);
    }
}

TEST_F(OutputControlTest, RejectedCommandsPreserveAppliedLevels) {
    ASSERT_TRUE(state_.select_mode(4));
    ASSERT_TRUE(state_.set_outputs(10));
    metasurface::apply_outputs(state_, pins_);
    for (const char* command : {"M0", "M33", "T13", "101", "0102", "X"}) {
        ASSERT_FALSE(metasurface::process_command(command, std::strlen(command), state_));
        metasurface::apply_outputs(state_, pins_);
        expect_levels("0101");
    }
}

TEST_F(OutputControlTest, ListCommandPreservesAppliedLevels) {
    ASSERT_TRUE(state_.select_mode(4));
    ASSERT_TRUE(state_.set_outputs(10));
    metasurface::apply_outputs(state_, pins_);
    ListOutput output;
    ASSERT_TRUE(metasurface::process_command("LISTA", 5, state_, &output));
    EXPECT_FALSE(output.value.empty());
    metasurface::apply_outputs(state_, pins_);
    expect_levels("0101");
}

}
