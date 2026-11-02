#include "metasurface/controller_application.hpp"
#include "metasurface/output_control.hpp"

namespace metasurface {

ControllerApplication::ControllerApplication(DigitalOutput& pins, TextOutput* output)
    : pins_(pins), output_(output), state_(), receiver_(), initialized_(false) {}

void ControllerApplication::initialize() {
    initialized_ = false;
    state_ = ControllerState();
    receiver_ = LineReceiver();
    initialize_outputs(pins_);
    initialized_ = true;
}

ReceiveResult ControllerApplication::receive(char character) {
    if (!initialized_) {
        return ReceiveResult::Pending;
    }
    return receiver_.receive(character, state_, output_, &pins_);
}

void ControllerApplication::discard_line() {
    if (initialized_) {
        receiver_.discard_line();
    }
}

const ControllerState& ControllerApplication::state() const {
    return state_;
}

}
