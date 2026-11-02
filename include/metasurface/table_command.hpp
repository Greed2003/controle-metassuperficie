#ifndef METASURFACE_TABLE_COMMAND_HPP
#define METASURFACE_TABLE_COMMAND_HPP

#include <stddef.h>

namespace metasurface {

class ControllerState;

bool apply_table_command(const char* text, size_t length, ControllerState& state);

}

#endif
