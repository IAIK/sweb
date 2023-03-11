#pragma once

#include "Device.h"
#include "DeviceDriver.h"
#include "IrqDomain.h"
#include "ports.h"

#include "types.h"

class PIT : public Device, public IrqDomain
{
public:
    PIT();
    ~PIT() override = default;

    PIT(const PIT&) = delete;
    PIT& operator=(const PIT&) = delete;

    enum PITIoPort
    {
        PIT_PORT_CH_0_DATA = 0x40,
        PIT_PORT_CH_1_DATA = 0x41,
        PIT_PORT_CH_2_DATA = 0x42,
        PIT_PORT_COMMAND   = 0x43,
    };

    enum class OperatingMode : uint8_t
    {
        ONESHOT = 0,
        HW_RETRIGGERABLE_ONESHOT = 1,
        RATE_GENERATOR = 2,
        SQUARE_WAVE = 3,
        SOFTWARE_STROBE = 4,
        HARDWARE_STROBE = 5,
    };

    enum class AccessMode : uint8_t
    {
        LATCH_COUNT_VALUE = 0,
        LOW_BYTE_ONLY = 1,
        HIGH_BYTE_ONLY = 2,
        LOW_BYTE_HIGH_BYTE = 3,
    };

    struct [[gnu::packed]] PITCommandRegister
    {
        union
        {
            uint8 value;

            struct [[gnu::packed]]
            {
                uint8 bcd_mode       : 1;
                OperatingMode operating_mode : 3;
                AccessMode access_mode    : 2;
                uint8 channel : 2;
            };
        };
    };
    static_assert(sizeof(PITCommandRegister) == 1);


    static void init(PITCommandRegister command, uint16 divisor);

    static OperatingMode setOperatingMode(OperatingMode mode);
    static OperatingMode operatingMode();

    static void sendCommand(PITCommandRegister command);

    static uint16_t setFrequencyDivisor(uint16 reload_value);
    static uint16_t frequencyDivisor();

    static PIT& instance();


private:
    static OperatingMode operating_mode;
    static uint16_t frequency_divisor;

    using COMMAND_PORT = IoPort::StaticIoRegister<PIT_PORT_COMMAND, PITCommandRegister, false, true>;
    using CH0_DATA_PORT = IoPort::StaticIoRegister<PIT_PORT_CH_0_DATA, uint8_t, false, true>;
};

class PITDriver : public BasicDeviceDriver, public Driver<PIT>
{
public:
    PITDriver();
    ~PITDriver() override = default;

    static PITDriver& instance();

    void doDeviceDetection() override;
private:
};
