#include "metasurface/avr_digital_output.hpp"

#if defined(__AVR__)
#if !defined(__AVR_ATmega2560__)
#error "AvrDigitalOutput requires ATmega2560."
#endif
#include <avr/io.h>
#include <util/atomic.h>
#endif

namespace metasurface {
namespace {

void set_bit(volatile uint8_t& target, uint8_t bit, bool high) {
    const uint8_t mask = static_cast<uint8_t>(1u << bit);
#if defined(__AVR__)
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
#endif
    {
        const uint8_t value = target;
        target = high ? static_cast<uint8_t>(value | mask)
                      : static_cast<uint8_t>(value & static_cast<uint8_t>(~mask));
    }
}

}

#if defined(__AVR__)
AvrDigitalOutput::AvrDigitalOutput()
    : registers_{{DDRA, PORTA}, {DDRB, PORTB}, {DDRC, PORTC},
                 {DDRD, PORTD}, {DDRG, PORTG}, {DDRL, PORTL}} {}
#endif

AvrDigitalOutput::AvrDigitalOutput(const AvrGpioRegisters& registers)
    : registers_(registers) {}

AvrDigitalOutput::Pin AvrDigitalOutput::resolve_pin(uint8_t pin) const {
    if (pin >= 22 && pin <= 29) {
        return {&registers_.a, static_cast<uint8_t>(pin - 22)};
    }
    if (pin >= 30 && pin <= 37) {
        return {&registers_.c, static_cast<uint8_t>(37 - pin)};
    }
    if (pin == 38) {
        return {&registers_.d, 7};
    }
    if (pin >= 39 && pin <= 41) {
        return {&registers_.g, static_cast<uint8_t>(41 - pin)};
    }
    if (pin >= 42 && pin <= 49) {
        return {&registers_.l, static_cast<uint8_t>(49 - pin)};
    }
    if (pin >= 50 && pin <= 53) {
        return {&registers_.b, static_cast<uint8_t>(53 - pin)};
    }
    return {nullptr, 0};
}

void AvrDigitalOutput::write(uint8_t pin, bool high) {
    const Pin resolved = resolve_pin(pin);
    if (resolved.port != nullptr) {
        set_bit(resolved.port->output, resolved.bit, high);
    }
}

void AvrDigitalOutput::configure_output(uint8_t pin) {
    const Pin resolved = resolve_pin(pin);
    if (resolved.port != nullptr) {
        set_bit(resolved.port->direction, resolved.bit, true);
    }
}

}
