#ifndef METASURFACE_DIGITAL_OUTPUT_HPP
#define METASURFACE_DIGITAL_OUTPUT_HPP

#include <stdint.h>

namespace metasurface {

class DigitalOutput {
public:
    // Stores the level even before output configuration: false = LOW, true = HIGH.
    virtual void write(uint8_t pin, bool high) = 0;
    // Enables the output using the level previously written.
    virtual void configure_output(uint8_t pin) = 0;

protected:
    ~DigitalOutput() = default;
};

}

#endif
