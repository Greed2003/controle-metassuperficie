#ifndef METASURFACE_LINE_RECEIVER_HPP
#define METASURFACE_LINE_RECEIVER_HPP

#include "metasurface/mode.hpp"
#include <stddef.h>

namespace metasurface {

class ControllerState;
class TextOutput;

enum class ReceiveResult { Pending, Applied, Rejected };

class LineReceiver {
public:
    LineReceiver();
    ReceiveResult receive(char character, ControllerState& state,
                          TextOutput* output = nullptr);

private:
    void reset();

    char buffer_[kMaxChannels];
    size_t length_;
    bool discarding_;
    bool carriage_return_;
};

}

#endif
