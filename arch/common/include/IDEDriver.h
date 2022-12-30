#pragma once

#include "DeviceBus.h"
#include "DeviceDriver.h"
#include "IrqDomain.h"
#include "ports.h"
#include "source_location.h"
#include <cinttypes>
#include "ArchInterrupts.h"
#include "EASTL/array.h"
#include "EASTL/tuple.h"
#include "EASTL/vector.h"

class BDDriver;
class IDEControllerChannel;

// Description of an IDE device discovered during IDE bus device enumeration
// Used to find compatible IDE bus device drivers (e.g. PATADeviceDriver for PATA devices)
// Compatible drivers then create appropriate devices based on this description
struct IDEDeviceDescription
{
    IDEControllerChannel* controller;
    uint8_t device_num;

    struct Signature
    {
        uint8_t r_count;
        uint8_t r_lba_low;
        uint8_t r_lba_mid;
        uint8_t r_lba_high;

        friend bool operator==(const Signature& lhs, const Signature& rhs) = default;
    } signature;
};

class IDEControllerChannel : public DeviceBus<IDEDeviceDescription>,
                             public IrqDomain
{
public:

    struct IoRegister
    {
        struct [[gnu::packed]] DriveHead
        {
            union [[gnu::packed]]
            {
                struct [[gnu::packed]]
                {
                    uint8_t chs_head_0_3 : 4;
                    uint8_t drive_num    : 1;
                    uint8_t always_set_0 : 1;
                    uint8_t use_lba      : 1;
                    uint8_t always_set_1 : 1;
                };

                struct [[gnu::packed]]
                {
                    // alternative for chs_head_0_3 (union inside bitfield struct always
                    // uses at least one byte, so we have to do it the other way around
                    // instead)
                    uint8_t lba_24_27 : 4;
                    uint8_t _         : 4; // see above struct
                };

                uint8_t u8;
            };
        };

        static_assert(sizeof(DriveHead) == 1);

        struct [[gnu::packed]] Status
        {
            union [[gnu::packed]]
            {
                struct [[gnu::packed]]
                {
                    uint8_t error             : 1;
                    // Always zero
                    uint8_t index             : 1;
                    // Always zero
                    uint8_t corrected_data    : 1;
                    // Drive has PIO data ready to transfer or is ready to accept PIO data
                    uint8_t data_ready        : 1;
                    uint8_t srv               : 1;
                    uint8_t drive_fault_error : 1;
                    // 0 = error or drive spun down
                    uint8_t ready             : 1;
                    uint8_t busy              : 1;
                };

                uint8_t u8;
            };
        };

        static_assert(sizeof(Status) == 1);

        struct [[gnu::packed]] Error
        {
            union [[gnu::packed]]
            {
                struct [[gnu::packed]]
                {
                    // Illegal length indicator, command completion time out, cfa error bit
                    uint8_t illegal_length           : 1;
                    uint8_t end_of_media             : 1;
                    uint8_t command_aborted          : 1;
                    uint8_t media_change_request     : 1;
                    // invalid address
                    uint8_t id_not_found             : 1;
                    uint8_t media_changed            : 1;
                    uint8_t uncorrectable_data_error : 1;
                    // CRC error
                    uint8_t bad_block_detected       : 1;
                };

                uint8_t u8;
            };
        };

        static_assert(sizeof(Error) == 1);

        using DATA_t = IoPort::IoPortDescription<0, uint16_t, true, true>;
        using ERROR_t = IoPort::IoPortDescription<1, Error, true, false>;
        using FEATURES_t = IoPort::IoPortDescription<1, uint8_t, false, true>;
        using SECTOR_COUNT_t = IoPort::IoPortDescription<2, uint8_t, true, true>;
        using SECTOR_NUMBER_t = IoPort::IoPortDescription<3, uint8_t, true, true>;
        using LBA_LOW_t = SECTOR_NUMBER_t;
        using CYLINDER_LOW_t = IoPort::IoPortDescription<4, uint8_t, true, true>;
        using LBA_MID_t = CYLINDER_LOW_t;
        using CYLINDER_HIGH_t = IoPort::IoPortDescription<5, uint8_t, true, true>;
        using LBA_HIGH_t = CYLINDER_HIGH_t;
        using DRIVE_HEAD_t = IoPort::IoPortDescription<6, DriveHead, true, true>;
        using STATUS_t = IoPort::IoPortDescription<7, Status, true, false>;
        using COMMAND_t = IoPort::IoPortDescription<7, uint8_t, false, true>;

        static constexpr DATA_t DATA{};
        static constexpr ERROR_t ERROR{};
        static constexpr FEATURES_t FEATURES{};
        static constexpr SECTOR_COUNT_t SECTOR_COUNT{};
        static constexpr SECTOR_NUMBER_t SECTOR_NUMBER{};
        static constexpr LBA_LOW_t LBA_LOW{};
        static constexpr CYLINDER_LOW_t CYLINDER_LOW{};
        static constexpr LBA_MID_t LBA_MID{};
        static constexpr CYLINDER_HIGH_t CYLINDER_HIGH{};
        static constexpr LBA_HIGH_t LBA_HIGH{};
        static constexpr DRIVE_HEAD_t DRIVE_HEAD{};
        static constexpr STATUS_t STATUS{};
        static constexpr COMMAND_t COMMAND{};
    };

    struct ControlRegister
    {
        struct [[gnu::packed]] DeviceControl
        {
            union [[gnu::packed]]
            {
                struct [[gnu::packed]]
                {
                    uint8_t always_zero : 1;
                    // 0 = interrupts enabled, 1 = interrupts disabled
                    uint8_t interrupt_disable : 1;
                    // Set to 1 for 5 us, then clear to 0 to reset all ATA drives on the
                    // bus
                    uint8_t reset                          : 1;
                    uint8_t reserved                       : 4;
                    uint8_t lba48_high_order_byte_readback : 1;
                };

                uint8_t u8;
            };
        };

        static_assert(sizeof(DeviceControl) == 1);

        struct [[gnu::packed]] DriveAddress
        {
            union [[gnu::packed]]
            {
                struct [[gnu::packed]]
                {
                    // 0 when drive 0 is selected
                    uint8_t drive0_select : 1;
                    // 0 when drive 1 is selected
                    uint8_t drive1_select : 1;
                    // ones-complement of currently selected head
                    uint8_t head_select   : 4;
                    // 0 when write is in progress
                    uint8_t write_gate    : 1;
                    uint8_t reserved      : 1;
                };

                uint8_t u8;
            };
        };

        static_assert(sizeof(DriveAddress) == 1);

        using ALT_STATUS_t = IoPort::IoPortDescription<0, IoRegister::Status, true, false>;
        using DEVICE_CONTROL_t = IoPort::IoPortDescription<0, DeviceControl, false, true>;
        using DRIVE_ADDRESS_t = IoPort::IoPortDescription<1, DriveAddress, true, false>;

        static constexpr ALT_STATUS_t ALT_STATUS{};
        static constexpr DEVICE_CONTROL_t DEVICE_CONTROL{};
        static constexpr DRIVE_ADDRESS_t DRIVE_ADDRESS{};
    };

public:
    IDEControllerChannel(const eastl::string& name, uint16_t io_reg_base, uint16_t control_reg_base, uint16_t isa_irqnum);

    ~IDEControllerChannel() override = default;

    static bool detectChannel(uint16_t io_reg_base, uint16_t control_reg_base);

    void doDeviceDetection();

    void detectDrive(uint8_t drive_num);

    void selectDrive(uint8_t drive, bool force = false);

    uint8_t selectedDrive();

    void sendCommand(uint8_t);

    // Resets BOTH drives on the channel
    void reset();

    bool waitNotBusy(source_location loc = source_location::current());
    bool waitDataReady(source_location loc = source_location::current());
    bool waitDriveReady(source_location loc = source_location::current());

    [[nodiscard]] constexpr uint8_t isaIrqNumber() const { return isa_irqnum; }


    IoPort::IoRegisterSet io_regs;
    IoPort::IoRegisterSet control_regs;
    uint8_t isa_irqnum;
    size_t controller_id;
    static size_t num_ide_controllers;

    uint8_t selected_drive = 0;
};

class IDEController : public Device
{
public:
    IDEController(const eastl::string& name = "IDE Controller");
    ~IDEController() override = default;

    void doDeviceDetection();

    struct DefaultPorts
    {
        static constexpr uint16_t PRIMARY_IO = 0x1F0;
        static constexpr uint16_t PRIMARY_CONTROL = 0x3F6;
        static constexpr uint8_t PRIMARY_ISA_IRQ = 14;

        static constexpr uint16_t SECONDARY_IO = 0x170;
        static constexpr uint16_t SECONDARY_CONTROL = 0x376;
        static constexpr uint8_t SECONDARY_ISA_IRQ = 15;
    };

private:
    eastl::vector<IDEControllerChannel*> channels;
};

class IDEControllerDriver : public BasicDeviceDriver, public Driver<IDEController>
{
public:
    IDEControllerDriver();
    IDEControllerDriver(const IDEControllerDriver&) = delete;
    ~IDEControllerDriver() override = default;

    static IDEControllerDriver& instance();

    static int32_t detectPartitions(BDDriver*, uint32_t, uint32_t, const char*);

    void doDeviceDetection() override;
};
