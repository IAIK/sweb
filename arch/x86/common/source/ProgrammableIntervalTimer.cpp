#include "ProgrammableIntervalTimer.h"
#include "ports.h"
#include "assert.h"

void PIT::init(uint8 command, uint16 divisor)
{
        assert(((PITCommandRegister*)&command)->access_mode == 3);
        sendCommand(command);
        setFrequencyDivisor(divisor);
}

void PIT::sendCommand(uint8 command)
{
        outportb(PIT_PORT_COMMAND, command);
}

void PIT::setFrequencyDivisor(uint16 reload_value)
{
        outportb(PIT_PORT_CH_0_DATA, reload_value & 0xFF);
        outportb(PIT_PORT_CH_0_DATA, reload_value >> 8);
}
