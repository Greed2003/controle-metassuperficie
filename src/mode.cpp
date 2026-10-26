#include "metasurface/mode.hpp"

namespace metasurface {

bool is_valid_mode(int mode) {
    return mode >= 1 && mode <= kMaxChannels;
}

}
