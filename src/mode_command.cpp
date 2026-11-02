#include "metasurface/mode_command.hpp"
#include "metasurface/controller_state.hpp"

namespace metasurface {

bool apply_mode_command(const char* text, size_t length, ControllerState& state) {
    if (text == nullptr || length < 2 || length > 3) {
        return false;
    }

    if (text[0] != 'M' || text[1] == '0') {
        return false;
    }

    int mode = 0;
    for (size_t index = 1; index < length; ++index) {
        if (text[index] < '0' || text[index] > '9') {
            return false;
        }
        mode = mode * 10 + (text[index] - '0');
    }

    return state.select_mode(mode);
}

}
