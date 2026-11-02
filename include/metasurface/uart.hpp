#ifndef METASURFACE_UART_HPP
#define METASURFACE_UART_HPP

#include <stdint.h>

namespace metasurface {

enum class UartReadResult { Empty, Received, Error };

class Uart {
public:
    virtual void initialize() = 0;
    // Nonblocking. Error consumes a byte; only Received changes value.
    virtual UartReadResult try_read(uint8_t& value) = 0;
    // Nonblocking. True means accepted, not necessarily transmitted on the wire.
    virtual bool try_write(uint8_t value) = 0;

protected:
    ~Uart() = default;
};

}

#endif
