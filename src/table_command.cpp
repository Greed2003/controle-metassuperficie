#include "metasurface/table_command.hpp"
#include "metasurface/controller_state.hpp"
#include "metasurface/pattern_catalog.hpp"
#include "numbered_command.hpp"

namespace metasurface {

bool apply_table_command(const char* text, size_t length, ControllerState& state) {
    int table = 0;
    uint32_t outputs = 0;
    if (!detail::parse_numbered_command(text, length, 'T', table)
        || !get_pattern(state.mode(), table, outputs)) {
        return false;
    }
    return state.set_outputs(outputs);
}

}
