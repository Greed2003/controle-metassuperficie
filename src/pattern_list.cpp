#include "metasurface/pattern_list.hpp"
#include "metasurface/mode.hpp"
#include "metasurface/pattern_catalog.hpp"
#include "metasurface/text_output.hpp"

namespace metasurface {
namespace {

size_t write_prefix(char* buffer, char prefix, int number) {
    size_t length = 0;
    buffer[length++] = prefix;
    if (number >= 10) {
        buffer[length++] = static_cast<char>('0' + number / 10);
    }
    buffer[length++] = static_cast<char>('0' + number % 10);
    return length;
}

}

bool write_pattern_list(int mode, TextOutput& output) {
    const int count = pattern_count(mode);
    if (count == 0) {
        return false;
    }

    char line[kMaxChannels + 6]; // T12: followed by up to 32 bits and LF.
    size_t length = write_prefix(line, 'M', mode);
    line[length++] = '\n';
    output.write(line, length);

    for (int table = 1; table <= count; ++table) {
        uint32_t outputs = 0;
        if (!get_pattern(mode, table, outputs)) {
            return false;
        }
        length = write_prefix(line, 'T', table);
        line[length++] = ':';
        line[length++] = ' ';
        for (int channel = 0; channel < mode; ++channel) {
            line[length++] = (outputs & (uint32_t{1} << channel)) ? '1' : '0';
        }
        line[length++] = '\n';
        output.write(line, length);
    }
    return true;
}

}
