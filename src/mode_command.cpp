#include "metasurface/mode_command.hpp"
#include "metasurface/controller_state.hpp"
#include "numbered_command.hpp"

namespace metasurface {

bool apply_mode_command(const char* text, size_t length, ControllerState& state) {
    int mode = 0;
    return detail::parse_numbered_command(text, length, 'M', mode)
        && state.select_mode(mode);
}

}
