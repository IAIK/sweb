#pragma once

#include "DeviceDriver.h"
#include "IrqDomain.h"
#include "chardev.h"
#include "types.h"
#include "ArchSerialInfo.h"


class ArchSerialInfo;

class SerialPort : public CharacterDevice, public IrqDomain
{
  public:
      enum BAUD_RATE_E
      {
          BR_9600,
          BR_14400,
          BR_19200,
          BR_38400,
          BR_55600,
          BR_115200
      };

      enum PARITY_E
      {
          ODD_PARITY,
          EVEN_PARITY,
          NO_PARITY
      };

      enum DATA_BITS_E
      {
          DATA_7,
          DATA_8
      };

      enum STOP_BITS_E
      {
          STOP_ONE,
          STOP_TWO,
          STOP_ONEANDHALF
      };

      enum SRESULT
      {
          SR_OK,
          SR_ERROR // you might want to add elements for common errors that can appear
      };

      SerialPort(const char* name, const ArchSerialInfo& port_info);

      ~SerialPort() override = default;

      /**
       * Opens a serial port for reading or writing
       * @param baud_rate Speed @see BAUD_RATE_E
       * @param data_bits Data bits @see DATA_BITS_E
       * @param stop_bits Stop bits @see STOP_BITS_E
       * @param parity Parity @see PARITY_E
       * @return Result @see SERIAL_ERROR_E
       */
      SRESULT setup_port(BAUD_RATE_E baud_rate,
                         DATA_BITS_E data_bits,
                         STOP_BITS_E stop_bits,
                         PARITY_E parity);

      /**
       * Writes size bytes to serial port
       * @param offset Not used with serial ports
       * @param size Number of bytes to be written
       * @param buffer The data to be written
       * @return Number of bytes actualy written or -1 in case of an error
       */
      int32 writeData(uint32 offset, uint32 size, const char* buffer) override;

      void irq_handler();

      /**
       * Returns the Architecture specific data for this serial port.
       * The basic task of any operating system is to hide this ugly data.
       * This function is mainly called from SerialManager and I do not think
       * that it is needed anywhere else. Perhaps it could be protected and
       * SerialManager declared as a friend class.
       * @return @see ArchSerialInfo
       */
      [[nodiscard]] ArchSerialInfo get_info() const { return port_info_; }

    static void write_UART(uint16_t base_port, SerialPortRegister reg, uint8 what);
    static uint8 read_UART(uint16_t base_port, SerialPortRegister reg);

private:
    void write_UART(SerialPortRegister reg, uint8 what);
    uint8 read_UART(SerialPortRegister reg);

    void readReceiveBuffers();
    size_t writeTransmitBuffer(const char* buffer, size_t size);

    size_t WriteLock;
    size_t SerialLock;

  private:
    ArchSerialInfo port_info_;
};

class SerialManager : public BasicDeviceDriver, public Driver<SerialPort>
{
public:
    SerialManager();
    ~SerialManager() override = default;

    static SerialManager& instance()
    {
        static SerialManager i;
        return i;
    }

    static constexpr size_t MAX_PORTS = 16;

    SerialPort* serial_ports[MAX_PORTS];

    uint32 do_detection(uint32 is_paging_set_up);
    [[nodiscard]] uint32 get_num_ports() const;
    [[nodiscard]] uint32 get_port_number(const uint8* friendly_name);
    void service_irq(uint32 irq_num);

    void doDeviceDetection() override;

private:
    uint32 num_ports;
};
