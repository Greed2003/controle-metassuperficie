#include "metasurface/avr_uart.hpp"

#if defined(__AVR__)
#if !defined(__AVR_ATmega2560__)
#error "AvrUart requires ATmega2560."
#endif
#if !defined(F_CPU) || F_CPU != 16000000UL
#error "AvrUart requires F_CPU=16000000UL."
#endif
#include <avr/io.h>
#include <util/atomic.h>
#endif

namespace metasurface {
namespace {

constexpr uint8_t kPowerDown = 0x02;
constexpr uint8_t kReceiveComplete = 0x80;
constexpr uint8_t kTransmitBufferEmpty = 0x20;
constexpr uint8_t kReceiveErrors = 0x1c;
constexpr uint8_t kDoubleSpeed = 0x02;
constexpr uint8_t kReceiverEnable = 0x10;
constexpr uint8_t kTransmitterEnable = 0x08;
constexpr uint8_t kEightDataBits = 0x06;
constexpr uint8_t kBaudDivider = 16;

}

#if defined(__AVR__)
AvrUart::AvrUart()
    : registers_{PRR0, UCSR0A, UCSR0B, UCSR0C, UBRR0H, UBRR0L, UDR0} {}
#endif

AvrUart::AvrUart(const AvrUartRegisters& registers)
    : registers_(registers) {}

void AvrUart::initialize() {
#if defined(__AVR__)
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
#endif
    {
        registers_.power_reduction &= static_cast<uint8_t>(~kPowerDown);
        registers_.control_b = 0;
        registers_.baud_high = 0;
        registers_.baud_low = kBaudDivider;
        // Do not copy status flags: TXC is cleared by writing one.
        registers_.status_a = kDoubleSpeed;
        registers_.control_c = kEightDataBits;
        registers_.control_b = kReceiverEnable | kTransmitterEnable;
    }
}

UartReadResult AvrUart::try_read(uint8_t& value) {
    if ((registers_.power_reduction & kPowerDown) != 0 ||
        (registers_.control_b & kReceiverEnable) == 0) {
        return UartReadResult::Empty;
    }
    const uint8_t status = registers_.status_a;
    if ((status & kReceiveComplete) == 0) {
        return UartReadResult::Empty;
    }
    // Status must be sampled before UDR advances the receive FIFO.
    const uint8_t received = registers_.data;
    if ((status & kReceiveErrors) != 0) {
        return UartReadResult::Error;
    }
    value = received;
    return UartReadResult::Received;
}

bool AvrUart::try_write(uint8_t value) {
    if ((registers_.power_reduction & kPowerDown) != 0 ||
        (registers_.control_b & kTransmitterEnable) == 0 ||
        (registers_.status_a & kTransmitBufferEmpty) == 0) {
        return false;
    }
    registers_.data = value;
    return true;
}

}
