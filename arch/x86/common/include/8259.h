#pragma once

#include "types.h"
#include "ports.h"
#include "IrqDomain.h"
#include "DeviceDriver.h"
#include "Device.h"

class PIC8259 : public InterruptController, public IrqDomain, public Device
{
public:
  static bool exists;
  static bool enabled;

  PIC8259();

  static PIC8259& instance();

  void init();


  virtual bool mask(irqnum_t irq, bool mask);
  virtual bool ack(irqnum_t irq);
  virtual bool irqStart(irqnum_t irq);

/**
 * sends the initialisation and operational command words to CPU
 *
 */
  static void initialise8259s();


  static uint16 cached_mask;

/**
 * enables the interrupt Request with the given number 0 to 15
 *
 */
  static void enableIRQ(uint16 number);

/**
 * disables the interrupt Request with the given number 0 to 15
 *
 */
  static void disableIRQ(uint16 number);

  static void setIrqMask(uint16 mask);

  static bool isIRQEnabled(uint16 number);

/**
 * sends the EOI signal to a Programmable Interrupt Controller (PIC)
 * to indicate the completion of interrupt processing for the given
 * interrupt.
 *
 */
  static void sendEOI(uint16 number);

  static size_t outstanding_EOIs_;

private:
    void setupIrqMappings();

    enum class IoPorts : uint16_t
    {
        PIC_1_CONTROL_PORT = 0x20,
        PIC_1_DATA_PORT    = 0x21,
        PIC_2_CONTROL_PORT = 0xA0,
        PIC_2_DATA_PORT    = 0xA1,
    };

    using PIC_1_CONTROL_PORT = IoPort::StaticIoRegister<(uint16_t)IoPorts::PIC_1_CONTROL_PORT, uint8_t, false, true>;
    using PIC_2_CONTROL_PORT = IoPort::StaticIoRegister<(uint16_t)IoPorts::PIC_2_CONTROL_PORT, uint8_t, false, true>;
    using PIC_1_DATA_PORT = IoPort::StaticIoRegister<(uint16_t)IoPorts::PIC_1_DATA_PORT, uint8_t, false, true>;
    using PIC_2_DATA_PORT = IoPort::StaticIoRegister<(uint16_t)IoPorts::PIC_2_DATA_PORT, uint8_t, false, true>;
};

class PIC8259Driver : public BasicDeviceDriver, public Driver<PIC8259>
{
public:
    PIC8259Driver();
    ~PIC8259Driver() override = default;

    static PIC8259Driver& instance();

    void doDeviceDetection() override;
private:
};
