#include "metasurface/command_processor.hpp"
#include "metasurface/binary_input.hpp"
#include "metasurface/controller_state.hpp"
#include "metasurface/mode_command.hpp"
#include "metasurface/table_command.hpp"
#include "metasurface/pattern_list.hpp"
#include <string.h>

namespace metasurface {

bool process_command(const char* text, size_t length, ControllerState& state,
                     TextOutput* output) {
    if (text == nullptr || length == 0) {
        return false;
    }

    if (text[0] == 'M') {
        return apply_mode_command(text, length, state);
    }

    if (text[0] == 'T') {
        return apply_table_command(text, length, state);
    }

    if (length == 5 && memcmp(text, "LISTA", 5) == 0) {
        return output != nullptr && write_pattern_list(state.mode(), *output);
    }

    uint32_t outputs = 0;
    if (!parse_binary_input(text, length, state.mode(), outputs)) {
        return false;
    }

    return state.set_outputs(outputs);
}

}
