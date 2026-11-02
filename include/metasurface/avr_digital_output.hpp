#ifndef METASURFACE_AVR_DIGITAL_OUTPUT_HPP
#define METASURFACE_AVR_DIGITAL_OUTPUT_HPP

#include "metasurface/digital_output.hpp"

namespace metasurface {

struct AvrPort {
    volatile uint8_t& direction;
    volatile uint8_t& output;
};

struct AvrGpioRegisters {
    AvrPort a;
    AvrPort b;
    AvrPort c;
    AvrPort d;
    AvrPort g;
    AvrPort l;
};

class AvrDigitalOutput final : public DigitalOutput {
public:
#if defined(__AVR__)
    AvrDigitalOutput();
#endif
    // Referenced registers must outlive this object.
    explicit AvrDigitalOutput(const AvrGpioRegisters& registers);
    void write(uint8_t pin, bool high) override;
    void configure_output(uint8_t pin) override;

private:
    struct Pin {
        const AvrPort* port;
        uint8_t bit;
    };
    Pin resolve_pin(uint8_t pin) const;
    const AvrGpioRegisters registers_;
};

}

#endif
