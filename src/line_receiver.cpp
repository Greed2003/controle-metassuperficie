#include "metasurface/line_receiver.hpp"
#include "metasurface/command_processor.hpp"
#include "command_response.hpp"
#include "metasurface/controller_state.hpp"

namespace metasurface {

LineReceiver::LineReceiver()
    : buffer_{}, length_(0), discarding_(false), carriage_return_(false) {}

ReceiveResult LineReceiver::receive(char character, ControllerState& state,
                                    TextOutput* output, DigitalOutput* pins) {
    if (character == '\n') {
        ReceiveResult result = ReceiveResult::Pending;
        if (discarding_) {
            write_command_response(CommandResponse::InvalidLine, state.mode(), output);
            result = ReceiveResult::Rejected;
        } else if (length_ > 0) {
            result = process_command(buffer_, length_, state, output, pins)
                ? ReceiveResult::Applied : ReceiveResult::Rejected;
        }
        reset();
        return result;
    }

    if (discarding_) {
        return ReceiveResult::Pending;
    }

    if (carriage_return_) {
        discard_line();
        return ReceiveResult::Pending;
    }

    if (character == '\r') {
        carriage_return_ = true;
        return ReceiveResult::Pending;
    }

    if (length_ == sizeof(buffer_)) {
        discard_line();
        return ReceiveResult::Pending;
    }

    buffer_[length_++] = character;
    return ReceiveResult::Pending;
}

void LineReceiver::discard_line() {
    reset();
    discarding_ = true;
}

void LineReceiver::reset() {
    length_ = 0;
    discarding_ = false;
    carriage_return_ = false;
}

}
