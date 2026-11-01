#include "metasurface/binary_input.hpp"
#include "metasurface/mode.hpp"

namespace metasurface {

bool parse_binary_input(const char* text, size_t length, int mode, uint32_t& outputs) {
    if (text == nullptr || !is_valid_mode(mode) || length != static_cast<size_t>(mode)) {
        return false;
    }

    uint32_t parsed_outputs = 0;

    for (size_t channel = 0; channel < length; ++channel) {
        if (text[channel] == '1') {
            parsed_outputs |= uint32_t{1} << channel;
        } else if (text[channel] != '0') {
            return false;
        }
    }

    outputs = parsed_outputs;
    return true;
}

}
