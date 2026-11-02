#ifndef METASURFACE_PATTERN_CATALOG_HPP
#define METASURFACE_PATTERN_CATALOG_HPP

#include <stdint.h>

namespace metasurface {

int pattern_count(int mode);
bool get_pattern(int mode, int table, uint32_t& outputs);

}

#endif
