#ifndef METASURFACE_CONTROLLER_APPLICATION_HPP
#define METASURFACE_CONTROLLER_APPLICATION_HPP

#include "metasurface/controller_state.hpp"
#include "metasurface/line_receiver.hpp"

namespace metasurface {

class DigitalOutput;
class TextOutput;

class ControllerApplication {
public:
    // Dependencies must outlive the application; construction performs no I/O.
    explicit ControllerApplication(DigitalOutput& pins, TextOutput* output = nullptr);
    ControllerApplication(const ControllerApplication&) = delete;
    ControllerApplication& operator=(const ControllerApplication&) = delete;

    void initialize();
    // Input is ignored until initialize; pass only bytes received without errors.
    ReceiveResult receive(char character);
    void discard_line();
    const ControllerState& state() const;

private:
    DigitalOutput& pins_;
    TextOutput* output_;
    ControllerState state_;
    LineReceiver receiver_;
    bool initialized_;
};

}

#endif
