#ifndef METASURFACE_SERIAL_TRANSPORT_HPP
#define METASURFACE_SERIAL_TRANSPORT_HPP

#include "metasurface/controller_application.hpp"
#include "metasurface/text_output.hpp"
#include "metasurface/uart.hpp"

namespace metasurface {

constexpr size_t kSerialReceiveCapacity = 64;
constexpr size_t kSerialResponseCapacity = 512;

class SerialTransport : private TextOutput {
public:
    // Dependencies must outlive the transport; construction performs no I/O.
    SerialTransport(Uart& uart, DigitalOutput& pins);
    SerialTransport(const SerialTransport&) = delete;
    SerialTransport& operator=(const SerialTransport&) = delete;

    void initialize();
    // One read, at most one queued input and one write attempt per call.
    void poll();
    const ControllerState& state() const;
    bool response_pending() const;
    bool faulted() const;

private:
    struct Input { uint8_t value; bool error; };
    void write(const char* text, size_t length) override;

    Uart& uart_;
    ControllerApplication application_;
    Input input_[kSerialReceiveCapacity];
    char response_[kSerialResponseCapacity];
    size_t input_head_;
    size_t input_count_;
    size_t response_size_;
    size_t response_position_;
    bool initialized_;
    bool faulted_;
};

}

#endif
