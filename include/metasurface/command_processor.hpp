#ifndef METASURFACE_COMMAND_PROCESSOR_HPP
#define METASURFACE_COMMAND_PROCESSOR_HPP

#include <stddef.h>

namespace metasurface {

class ControllerState;

bool process_command(const char* text, size_t length, ControllerState& state);

}

#endif
