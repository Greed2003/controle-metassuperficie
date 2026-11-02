#ifndef METASURFACE_MODE_COMMAND_HPP
#define METASURFACE_MODE_COMMAND_HPP

#include <stddef.h>

namespace metasurface {

class ControllerState;

bool apply_mode_command(const char* text, size_t length, ControllerState& state);

}

#endif
