#include "KeyboardManager.h"
#include "kprintf.h"
#include "board_constants.h"
#include "Console.h"

uint32 const KeyboardManager::STANDARD_KEYMAP[KEY_MAPPING_SIZE] = STANDARD_KEYMAP_DEF;

uint32 const KeyboardManager::E0_KEYS[KEY_MAPPING_SIZE] = E0_KEYS_DEF;

KeyboardManager *KeyboardManager::instance_ = 0;

extern struct KMI* kmi;

KeyboardManager::KeyboardManager() :
    keyboard_buffer_(256), extended_scancode(0), keyboard_status_(0), usb_kbd_addr_(0), current_key_(0)
{
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

void KeyboardManager::serviceIRQ( void )
{
  while (*(volatile unsigned long*)(SERIAL_BASE + SERIAL_FLAG_REGISTER) & (SERIAL_BUFFER_FULL))
  {
    if(main_console)
    {
      keyboard_buffer_.put(*(volatile unsigned long*)SERIAL_BASE); // put it inside the buffer
    }
  }
}

void KeyboardManager::modifyKeyboardStatus(uint8 sc)
{
  bool key_released = sc & 0200;

  if( key_released )
    sc &= 0x7f;

  uint32 key = convertScancode( sc );

  uint32 simple_key = key & 0xFF;
  uint32 control_key = key & 0xFF00;

  if( simple_key )
    return;

  if( key_released )
  {
    if(!((control_key & KBD_META_CAPS) || (control_key & KBD_META_NUM) || (control_key & KBD_META_SCRL)))
      keyboard_status_ &= ~control_key;
  }
  else
  {
    if(control_key & KBD_META_CAPS)
    {
      keyboard_status_ ^= KBD_META_CAPS;
    }
    else if(control_key & KBD_META_NUM)
    {
      keyboard_status_ ^= KBD_META_NUM;
    }
    else if(control_key & KBD_META_SCRL)
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


void KeyboardManager::setLEDs( void )
{
  static uint32 last_leds = 0;
  uint32 leds = 0;
  if(keyboard_status_ & KBD_META_SCRL)
    leds |= 1;
  if(keyboard_status_ & KBD_META_NUM)
    leds |= 2;
  if(keyboard_status_ & KBD_META_CAPS)
    leds |= 4;
  if(last_leds != leds)
  {
    send_cmd( 0xF4 );  // enable keyboard command
    send_cmd( 0xED );  // "set LEDs" command
    send_cmd( leds );  // bottom 3 bits set LEDs
    last_leds = leds;
  }
}

uint32 KeyboardManager::convertScancode( uint8 scancode )
{
  return scancode;
}
