#ifndef METASURFACE_AVR_UART_HPP
#define METASURFACE_AVR_UART_HPP

#include <stdint.h>

namespace metasurface {

struct AvrUartRegisters {
    volatile uint8_t& power_reduction;
    volatile uint8_t& status_a;
    volatile uint8_t& control_b;
    volatile uint8_t& control_c;
    volatile uint8_t& baud_high;
    volatile uint8_t& baud_low;
    volatile uint8_t& data;
};

enum class UartReadResult { Empty, Received, Error };

class AvrUart {
public:
#if defined(__AVR__)
    AvrUart();
#endif
    // Referenced registers must outlive this object.
    explicit AvrUart(const AvrUartRegisters& registers);
    // Call before communication: UART0, 16 MHz, nominal 115200 baud, 8N1.
    void initialize();
    // Error consumes the faulty byte; only Received changes value.
    UartReadResult try_read(uint8_t& value);
    // True means accepted by the transmit buffer, not fully transmitted.
    bool try_write(uint8_t value);

private:
    const AvrUartRegisters registers_;
};

}

#endif
