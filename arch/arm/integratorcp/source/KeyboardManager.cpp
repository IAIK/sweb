/**
 * @file arch_keyboard_manager.cpp
 *
 */

#include "KeyboardManager.h"
#include "kprintf.h"
#include "Console.h"

uint32 const KeyboardManager::STANDARD_KEYMAP[KEY_MAPPING_SIZE] = STANDARD_KEYMAP_DEF;

uint32 const KeyboardManager::E0_KEYS[KEY_MAPPING_SIZE] = E0_KEYS_DEF;

KeyboardManager *KeyboardManager::instance_ = 0;

extern struct KMI* kmi;

KeyboardManager::KeyboardManager() :
    keyboard_buffer_(256), extended_scancode(0), keyboard_status_(0), usb_kbd_addr_(0), current_key_(0)
{
  kmi = (struct KMI*) 0x88000000;
  kmi->cr = 0x1C;
  kmi->data = 0xF4;
}

KeyboardManager::~KeyboardManager()
{
}

void KeyboardManager::kb_wait()
{
}

void KeyboardManager::send_cmd(uint8 cmd, uint8 port __attribute__((unused)))
{
  kb_wait();
  kmi->data = cmd;
}

void KeyboardManager::serviceIRQ(void)
{
  uint8 scancode = kmi->data;
  if (scancode > 0x80)
    return;

  if (extended_scancode == 0xE0)
  {
    if (scancode == 0x2A || scancode == 0x36 || scancode >= E0_BASE)
    {
      extended_scancode = 0;
    }

    scancode = E0_KEYS[scancode];
  }
  else if (extended_scancode == 0xE1 && scancode == 0x1D)
  {
    extended_scancode = 0x100;
  }
  else if (extended_scancode == 0x100 && scancode == 0x45)
    scancode = E1_PAUSE;

  extended_scancode = 0;

  if (scancode == 0xFF || scancode == 0xFA || scancode == 0xFE || scancode == 0x00) // non parsable codes, ACK and keyb. buffer errors
  {
    debug(A_KB_MANAGER, "Non-parsable scancode %X \n", scancode);
  }

  if (scancode == 0xE0 || scancode == 0xE1)
  {
    extended_scancode = scancode;
  }

  modifyKeyboardStatus(scancode);
  setLEDs(); // setting the leds
  if (main_console)
  {
    keyboard_buffer_.put(scancode); // put it inside the buffer
    main_console->addJob();
  }

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
  return;
}

void KeyboardManager::emptyKbdBuffer()
{
  while (kbdBufferFull())
    kbdGetScancode();
}

void KeyboardManager::setLEDs(void)
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
    send_cmd(0xF4); // enable keyboard command
    send_cmd(0xED); // "set LEDs" command
    send_cmd(leds); // bottom 3 bits set LEDs
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
