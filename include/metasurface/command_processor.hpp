#ifndef METASURFACE_COMMAND_PROCESSOR_HPP
#define METASURFACE_COMMAND_PROCESSOR_HPP

#include <stddef.h>

namespace metasurface {

class ControllerState;
class DigitalOutput;
class TextOutput;

// Optional pins must be initialized; successful changes reach them before the reply.
bool process_command(const char* text, size_t length, ControllerState& state,
                     TextOutput* output = nullptr, DigitalOutput* pins = nullptr);

}

#endif
