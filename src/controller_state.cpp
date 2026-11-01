#include "metasurface/controller_state.hpp"
#include "metasurface/mode.hpp"

namespace metasurface {

ControllerState::ControllerState() : mode_(0), outputs_(0) {}

int ControllerState::mode() const {
    return mode_;
}

uint32_t ControllerState::outputs() const {
    return outputs_;
}

bool ControllerState::select_mode(int mode) {
    if (!is_valid_mode(mode)) {
        return false;
    }

    mode_ = mode;
    outputs_ = 0;
    return true;
}

bool ControllerState::set_outputs(uint32_t outputs) {
    if (!is_valid_mode(mode_)) {
        return false;
    }

    if (mode_ < kMaxChannels && (outputs >> mode_) != 0) {
        return false;
    }

    outputs_ = outputs;
    return true;
}

}
