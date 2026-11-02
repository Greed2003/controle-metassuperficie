#ifndef METASURFACE_COMMAND_PROCESSOR_HPP
#define METASURFACE_COMMAND_PROCESSOR_HPP

#include <stddef.h>

namespace metasurface {

class ControllerState;
class TextOutput;

bool process_command(const char* text, size_t length, ControllerState& state,
                     TextOutput* output = nullptr);

}

#endif
