#ifndef METASURFACE_TEXT_OUTPUT_HPP
#define METASURFACE_TEXT_OUTPUT_HPP

#include <stddef.h>

namespace metasurface {

class TextOutput {
public:
    // Consumes exactly length bytes before returning; no null terminator is required.
    virtual void write(const char* text, size_t length) = 0;

protected:
    ~TextOutput() = default;
};

}

#endif
