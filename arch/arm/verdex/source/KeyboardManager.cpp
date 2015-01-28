/**
 * @file arch_keyboard_manager.cpp
 *
 */

#include "KeyboardManager.h"
#include "kprintf.h"
#include "board_constants.h"
#include "Console.h"

  uint32 const KeyboardManager::STANDARD_KEYMAP[KEY_MAPPING_SIZE] =
  {
        0, 0x1B, '1', '2', '3', '4', '5' , '6',   // 08
        '7', '8', '9', '0', '-', '^', '\b', '\t', // 10
        'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',   // 18
        'o', 'p', '[', ']', '\n', KBD_META_CTRL, 'a', 's',  // 20
        'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',  // 28
        '\'', '`', KBD_META_SHIFT, '\\', 'z', 'x', 'c', 'v', // 30
        'b', 'n', 'm', ',', '.', '/',KBD_META_SHIFT, '*', // 38
        KBD_META_LALT, ' ', KBD_META_CAPS, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, // 40
        KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KBD_META_NUM, KBD_META_SCRL, '7', // 48
        '8', '9', '-', '4', '5', '6', '+', '1', // 50
        '2', '3', '0', '.', 0, 0, 0, KEY_F11  , // 58
        KEY_F12, 0, 0, 0, 0, 0, 0, 0, // 60
        0, '\n', KBD_META_CTRL, '/', KEY_PRNT, KBD_META_RALT, 0, KEY_HOME,  // 68
        KEY_UP, KEY_PGUP, KEY_LFT, KEY_RT, KEY_END, KEY_DN, KEY_PGDN, KEY_INS, // 70
        0, 0, 0, 0, 0, 0, 0, 0, // 78
        0, 0, 0, 0, 0, 0, 0, 0, // 80
  };

  uint32 const KeyboardManager::E0_KEYS[KEY_MAPPING_SIZE] =
  {
        0, 0, 0, 0, 0, 0, 0, 0, // 00
        0, 0, 0, 0, 0, 0, 0, 0, // 08
        0, 0, 0, 0, 0, 0, 0, 0, // 10
        0, 0, 0, 0, E0_KPENTER, E0_RCTRL, 0, 0, // 18
        0, 0, 0, 0, 0, 0, 0, 0, // 20
        0, 0, 0, 0, 0, 0, 0, 0, // 28
        0, 0, 0, 0, 0, E0_KPSLASH, 0, E0_PRSCR, // 30
        E0_RALT, 0, 0, 0, 0, 0, 0, 0, // 38
        0, 0, 0, 0, 0, 0, 0, E0_HOME,  // 40
        E0_UP, E0_PGUP, 0, E0_LEFT, 0, E0_RIGHT, 0, E0_END, // 48
        E0_DOWN, E0_PGDN, E0_INS, 0, 0, 0, 0, 0, // 50
        0, 0, 0, 0, 0, 0, 0, 0, // 58
        0, 0, 0, 0, 0, 0, 0, 0, // 60
        0, 0, 0, 0, 0, 0, 0, 0, // 68
        0, 0, 0, 0, 0, 0, 0, 0, // 70
        0, 0, 0, 0, 0, 0, 0, 0, // 78
  };

KeyboardManager *KeyboardManager::instance_ = 0;

extern struct KMI* kmi;

KeyboardManager::KeyboardManager() :
    keyboard_buffer_(256), extended_scancode(0), keyboard_status_(0), usb_kbd_addr_(0), current_key_(0)
{
//  kmi = (struct KMI*)0x88000000;
//  kmi->cr = 0x1C;
//  kmi->data = 0xF4;
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
      main_console->addJob();
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


bool KeyboardManager::isShift()
{
  return keyboard_status_ & KBD_META_SHIFT;
}

bool KeyboardManager::isCtrl()
{
  return keyboard_status_ & KBD_META_CTRL;
}

bool KeyboardManager::isAlt()
{
  return (keyboard_status_ & KBD_META_LALT) ;
}

bool KeyboardManager::isAltGr()
{
  return (keyboard_status_ & KBD_META_RALT) ;
}


bool KeyboardManager::isCaps()
{
  return (keyboard_status_ & KBD_META_CAPS) ;
}

bool KeyboardManager::isNum()
{
  return (keyboard_status_ & KBD_META_NUM) ;
}

bool KeyboardManager::isScroll()
{
  return (keyboard_status_ & KBD_META_SCRL) ;
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
  uint32 simple_key = STANDARD_KEYMAP[ scancode ] & 0xFF;
  uint32 control_key = STANDARD_KEYMAP[ scancode ] & 0xFF00;

  uint32 key = control_key | simple_key;
  return key;
}

bool KeyboardManager::getKeyFromKbd(uint32 &key)
{
  //peeking should not block
  uint8 sc;
  if (keyboard_buffer_.get(sc))
  {
    key = sc;
    return true;
  }
  else
    return false;
}
