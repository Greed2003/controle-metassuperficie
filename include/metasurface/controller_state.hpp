#ifndef METASURFACE_CONTROLLER_STATE_HPP
#define METASURFACE_CONTROLLER_STATE_HPP

#include <stdint.h>

namespace metasurface {

class ControllerState {
public:
    ControllerState();

    int mode() const;
    uint32_t outputs() const;

    bool select_mode(int mode);
    bool set_outputs(uint32_t outputs);

private:
    int mode_;
    uint32_t outputs_;
};

}

#endif
