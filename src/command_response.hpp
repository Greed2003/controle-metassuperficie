#ifndef METASURFACE_COMMAND_RESPONSE_HPP
#define METASURFACE_COMMAND_RESPONSE_HPP

namespace metasurface {

class TextOutput;

enum class CommandResponse {
    ModeSelected,
    TableApplied,
    OutputsApplied,
    ModeRequired,
    InvalidMode,
    InvalidTable,
    InvalidBinary,
    InvalidCommand,
    InvalidLine
};

void write_command_response(CommandResponse response, int mode, TextOutput* output);

}

#endif
