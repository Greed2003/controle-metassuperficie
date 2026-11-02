#include "metasurface/command_processor.hpp"
#include "metasurface/binary_input.hpp"
#include "command_response.hpp"
#include "metasurface/controller_state.hpp"
#include "metasurface/mode_command.hpp"
#include "metasurface/table_command.hpp"
#include "metasurface/pattern_list.hpp"
#include <string.h>

namespace metasurface {

bool process_command(const char* text, size_t length, ControllerState& state,
                     TextOutput* output) {
    if (text == nullptr || length == 0) {
        write_command_response(CommandResponse::InvalidCommand, state.mode(), output);
        return false;
    }

    if (text[0] == 'M') {
        const bool applied = apply_mode_command(text, length, state);
        write_command_response(applied ? CommandResponse::ModeSelected
                                       : CommandResponse::InvalidMode, state.mode(), output);
        return applied;
    }

    if (state.mode() == 0) {
        write_command_response(CommandResponse::ModeRequired, state.mode(), output);
        return false;
    }

    if (text[0] == 'T') {
        const bool applied = apply_table_command(text, length, state);
        write_command_response(applied ? CommandResponse::TableApplied
                                       : CommandResponse::InvalidTable, state.mode(), output);
        return applied;
    }

    if (length == 5 && memcmp(text, "LISTA", 5) == 0) {
        return output != nullptr && write_pattern_list(state.mode(), *output);
    }

    if (text[0] != '0' && text[0] != '1') {
        write_command_response(CommandResponse::InvalidCommand, state.mode(), output);
        return false;
    }

    uint32_t outputs = 0;
    const bool applied = parse_binary_input(text, length, state.mode(), outputs)
        && state.set_outputs(outputs);
    write_command_response(applied ? CommandResponse::OutputsApplied
                                   : CommandResponse::InvalidBinary, state.mode(), output);
    return applied;
}

}
