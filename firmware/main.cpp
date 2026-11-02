#include "metasurface/avr_digital_output.hpp"
#include "metasurface/avr_uart.hpp"
#include "metasurface/serial_transport.hpp"
#include <avr/interrupt.h>
#include <avr/wdt.h>

#if !defined(__AVR_ATmega2560__)
#error "This firmware requires ATmega2560."
#endif

// Runs before RAM initialization and C++ constructors.
extern "C" void disable_watchdog() __attribute__((naked, used, section(".init3")));
void disable_watchdog() {
    cli();
    MCUSR = 0;
    wdt_disable();
}

namespace {
metasurface::AvrDigitalOutput pins;
metasurface::AvrUart uart;
metasurface::SerialTransport transport(uart, pins);
}

int main() {
    cli();
    transport.initialize();
    for (;;) {
        transport.poll();
    }
}
