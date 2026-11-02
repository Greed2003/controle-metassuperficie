#include "metasurface/output_control.hpp"
#include "metasurface/controller_state.hpp"
#include "metasurface/digital_output.hpp"
#include "metasurface/mode.hpp"

namespace metasurface {
namespace {

constexpr uint8_t kFirstOutputPin = 22;

}

void initialize_outputs(DigitalOutput& output) {
    for (int channel = 0; channel < kMaxChannels; ++channel) {
        output.write(static_cast<uint8_t>(kFirstOutputPin + channel), false);
    }
    for (int channel = 0; channel < kMaxChannels; ++channel) {
        output.configure_output(static_cast<uint8_t>(kFirstOutputPin + channel));
    }
}

void apply_outputs(const ControllerState& state, DigitalOutput& output) {
    const uint32_t outputs = state.outputs();
    for (int channel = 0; channel < kMaxChannels; ++channel) {
        const bool high = (outputs & (uint32_t(1) << channel)) != 0;
        output.write(static_cast<uint8_t>(kFirstOutputPin + channel), high);
    }
}

}
