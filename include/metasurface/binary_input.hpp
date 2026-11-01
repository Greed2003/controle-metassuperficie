#ifndef METASURFACE_BINARY_INPUT_HPP
#define METASURFACE_BINARY_INPUT_HPP

#include <stddef.h>
#include <stdint.h>

namespace metasurface {

bool parse_binary_input(const char* text, size_t length, int mode, uint32_t& outputs);

}

#endif
