#ifndef METASURFACE_NUMBERED_COMMAND_HPP
#define METASURFACE_NUMBERED_COMMAND_HPP

#include <stddef.h>

namespace metasurface {
namespace detail {

inline bool parse_numbered_command(const char* text, size_t length,
                                   char prefix, int& value) {
    if (text == nullptr || length < 2 || length > 3) {
        return false;
    }
    if (text[0] != prefix || text[1] == '0') {
        return false;
    }

    int parsed = 0;
    for (size_t index = 1; index < length; ++index) {
        if (text[index] < '0' || text[index] > '9') {
            return false;
        }
        parsed = parsed * 10 + (text[index] - '0');
    }
    value = parsed;
    return true;
}

}
}

#endif
