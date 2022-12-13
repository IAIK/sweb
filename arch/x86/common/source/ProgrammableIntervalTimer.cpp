#include "ProgrammableIntervalTimer.h"
#include "ports.h"
#include "ArchInterrupts.h"
#include "ArchMulticore.h"
#include "assert.h"

PIT::OperatingMode PIT::operating_mode = PIT::OperatingMode::SQUARE_WAVE;
uint16_t PIT::frequency_divisor = 0;

PIT::PIT() :
    Device("Programmable Interval Timer"),
    IrqDomain("Programmable Interval Timer")
{
}

PIT& PIT::instance()
{
    static PIT i;
    return i;
}

void PIT::init(PITCommandRegister command, uint16 divisor)
{
    assert(command.access_mode == AccessMode::LOW_BYTE_HIGH_BYTE);
    sendCommand(command);
    setFrequencyDivisor(divisor);
}

void PIT::sendCommand(PITCommandRegister command)
{
    operating_mode = command.operating_mode;
    COMMAND_PORT::write(command);
}

PIT::OperatingMode PIT::setOperatingMode(PIT::OperatingMode mode)
{
    auto old_mode = operating_mode;

    sendCommand(PITCommandRegister{
        .bcd_mode = 0,
        .operating_mode = mode,
        .access_mode = PIT::AccessMode::LOW_BYTE_HIGH_BYTE, // send low + high byte of reload value/divisor
        .channel = 0,
    });

    return old_mode;
}

uint16_t PIT::setFrequencyDivisor(uint16 reload_value)
{
    auto old_freq = frequency_divisor;
    frequency_divisor = reload_value;
    assert(!(operating_mode == PIT::OperatingMode::SQUARE_WAVE && reload_value == 1));

    WithInterrupts i{false};
    CH0_DATA_PORT::write(reload_value & 0xFF);
    CH0_DATA_PORT::write(reload_value >> 8);

    return old_freq;
}

PIT::OperatingMode PIT::operatingMode()
{
    return operating_mode;
}

uint16_t PIT::frequencyDivisor()
{
    return frequency_divisor;
}

PITDriver::PITDriver() :
    base_type("PIT driver")
{
}

PITDriver& PITDriver::instance()
{
    static PITDriver i;
    return i;
}

extern "C" void arch_irqHandler_0();

void PITDriver::doDeviceDetection()
{
    bindDevice(PIT::instance());

    PIT::setOperatingMode(PIT::OperatingMode::SQUARE_WAVE);
    PIT::setFrequencyDivisor(0);

    PIT::instance().irq().mapTo(ArchInterrupts::isaIrqDomain(), 0);
}
