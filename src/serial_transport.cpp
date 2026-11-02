#include "metasurface/serial_transport.hpp"
#include <string.h>

namespace metasurface {

SerialTransport::SerialTransport(Uart& uart, DigitalOutput& pins)
    : uart_(uart), application_(pins, this), input_{}, response_{},
      input_head_(0), input_count_(0), response_size_(0), response_position_(0),
      initialized_(false), faulted_(false) {}

void SerialTransport::initialize() {
    initialized_ = false;
    input_head_ = 0;
    input_count_ = 0;
    response_size_ = 0;
    response_position_ = 0;
    faulted_ = false;
    application_.initialize();
    uart_.initialize();
    initialized_ = true;
}

void SerialTransport::poll() {
    if (!initialized_ || faulted_) {
        return;
    }

    uint8_t value = 0;
    const UartReadResult result = uart_.try_read(value);
    if (result != UartReadResult::Empty) {
        if (input_count_ == kSerialReceiveCapacity) {
            // Losing queued bytes invalidates the entire unfinished line.
            input_head_ = 0;
            input_count_ = 0;
            application_.discard_line();
        }
        const size_t tail = (input_head_ + input_count_) % kSerialReceiveCapacity;
        input_[tail] = Input{value, result == UartReadResult::Error};
        ++input_count_;
    }

    if (response_size_ == 0 && input_count_ != 0) {
        const Input input = input_[input_head_];
        input_head_ = (input_head_ + 1) % kSerialReceiveCapacity;
        --input_count_;
        if (input.error) {
            application_.discard_line();
        } else {
            application_.receive(static_cast<char>(input.value));
        }
    }

    if (!faulted_ && response_size_ != 0 &&
        uart_.try_write(static_cast<uint8_t>(response_[response_position_]))) {
        ++response_position_;
        if (response_position_ == response_size_) {
            response_position_ = 0;
            response_size_ = 0;
        }
    }
}

void SerialTransport::write(const char* text, size_t length) {
    if (faulted_) {
        return;
    }
    if (length > kSerialResponseCapacity - response_size_) {
        // A future oversized response must not be transmitted as a valid prefix.
        response_size_ = 0;
        response_position_ = 0;
        faulted_ = true;
        return;
    }
    if (length != 0) {
        memcpy(response_ + response_size_, text, length);
        response_size_ += length;
    }
}

const ControllerState& SerialTransport::state() const {
    return application_.state();
}

bool SerialTransport::response_pending() const {
    return response_size_ != 0;
}

bool SerialTransport::faulted() const {
    return faulted_;
}

}
