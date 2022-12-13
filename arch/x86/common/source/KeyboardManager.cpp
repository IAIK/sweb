#include "KeyboardManager.h"
#include "kprintf.h"
#include "Console.h"
#include "ports.h"

const uint32 KeyboardManager::STANDARD_KEYMAP[KEY_MAPPING_SIZE] = STANDARD_KEYMAP_DEF;

const uint32 KeyboardManager::E0_KEYS[KEY_MAPPING_SIZE] = E0_KEYS_DEF;

KeyboardManager::KeyboardManager() :
    IrqDomain("Keyboard"),
    keyboard_buffer_(256), extended_scancode(0), keyboard_status_(0)
{
  emptyKbdBuffer();
  IrqDomain::irq()
      .mapTo(ArchInterrupts::isaIrqDomain(), 1)
      .useHandler([this]{ serviceIRQ(); });
}

void KeyboardManager::kb_wait()
{
  uint32 i;

  for (i = 0; i < 0x10000; i++)
  {
    uint8 stat = inportb(0x64);
    if ((stat & 0x02) == 0)
      break;
  }
  if (i >= 0x10000)
    kprintfd("KeyboardManager::kb_wait: waiting on 0x02 didn't speed up things :-(\n");
}

void KeyboardManager::send_cmd(uint8 cmd, uint8 port)
{
  port = 0x64;
  kb_wait();
  outportbp(port, cmd);
}

void KeyboardManager::serviceIRQ()
{
  send_cmd(0xAD); // disable the keyboard
  kb_wait();

  uint8 scancode = inportb(0x60);

  if (extended_scancode == 0xE0)
  {
    if ((scancode == 0x2A || scancode == 0x36 || scancode >= E0_BASE) && !(scancode & KEY_MAPPING_SIZE))
    {
      extended_scancode = 0;
      send_cmd(0xAE); // enable the keyboard
      return;
    }

    if (scancode & KEY_MAPPING_SIZE)
      scancode = (E0_KEYS[scancode - KEY_MAPPING_SIZE]) + KEY_MAPPING_SIZE;
    else
      scancode = E0_KEYS[scancode];
  }
  else if (extended_scancode == 0xE1 && scancode == 0x1D)
  {
    extended_scancode = 0x100;
    send_cmd(0xAE); // enable the keyboard
    return;
  }
  else if (extended_scancode == 0x100 && scancode == 0x45)
    scancode = E1_PAUSE;

  extended_scancode = 0;

  if (scancode == 0xFF || scancode == 0xFA || scancode == 0xFE || scancode == 0x00) // non parsable codes, ACK and keyb. buffer errors
  {
    debug(A_KB_MANAGER, "Non-parsable scancode %X \n", scancode);
    send_cmd(0xAE); // enable the keyboard
    return;
  }

  if (scancode == 0xE0 || scancode == 0xE1)
  {
    extended_scancode = scancode;
    send_cmd(0xAE); // enable the keyboard
    return;
  }

  modifyKeyboardStatus(scancode);
  //setLEDs(); // setting the leds

  if ((scancode & 0200)) // if a key was released just ignore it
  {
    send_cmd(0xAE); // enable the keyboard
    return;
  }

  if (main_console)
  {
    keyboard_buffer_.put(scancode); // put it inside the buffer
  }

  send_cmd(0xAE); // enable the keyboard
}

void KeyboardManager::modifyKeyboardStatus(uint8 sc)
{
  bool key_released = sc & 0200;

  if (key_released)
    sc &= 0x7f;

  uint32 key = convertScancode(sc);

  uint32 simple_key = key & 0xFF;
  uint32 control_key = key & 0xFF00;

  if (simple_key)
    return;

  if (key_released)
  {
    if (!((control_key & KBD_META_CAPS) || (control_key & KBD_META_NUM) || (control_key & KBD_META_SCRL)))
      keyboard_status_ &= ~control_key;
  }
  else
  {
    if (control_key & KBD_META_CAPS)
    {
      keyboard_status_ ^= KBD_META_CAPS;
    }
    else if (control_key & KBD_META_NUM)
    {
      keyboard_status_ ^= KBD_META_NUM;
    }
    else if (control_key & KBD_META_SCRL)
    {
      keyboard_status_ ^= KBD_META_SCRL;
    }
    else
      keyboard_status_ |= control_key;
  }
}

void KeyboardManager::emptyKbdBuffer()
{
  while (kbdBufferFull())
    kbdGetScancode();
}

void KeyboardManager::setLEDs()
{
  static uint32 last_leds = 0;
  uint32 leds = 0;
  if (keyboard_status_ & KBD_META_SCRL)
    leds |= 1;
  if (keyboard_status_ & KBD_META_NUM)
    leds |= 2;
  if (keyboard_status_ & KBD_META_CAPS)
    leds |= 4;

  if (last_leds != leds)
  {
    send_cmd(0xF4, 0x60); // enable keyboard command
    send_cmd(0xED, 0x60); // "set LEDs" command
    send_cmd(leds, 0x60); // bottom 3 bits set LEDs
    last_leds = leds;
  }

}

uint32 KeyboardManager::convertScancode(uint8 scancode)
{
  uint32 simple_key = STANDARD_KEYMAP[scancode] & 0xFF;
  uint32 control_key = STANDARD_KEYMAP[scancode] & 0xFF00;

  uint32 key = control_key | simple_key;
  return key;
}
