// Projectname: SWEB
// Simple operating system for educational purposes
//
// Copyright (C) 2005  Nebojsa Simic
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

//----------------------------------------------------------------------
//   $Id: arch_keyboard_manager.cpp,v 1.8 2005/09/23 21:25:04 nelles Exp $
//----------------------------------------------------------------------
//
//  $Log: arch_keyboard_manager.cpp,v $
//  Revision 1.7  2005/09/21 17:01:12  nomenquis
//  updates
//
//  Revision 1.6  2005/09/21 15:37:02  btittelbach
//  full kbd buffer blocks irq1 bug fixed
//
//  Revision 1.5  2005/09/20 21:14:31  nelles
//
//
//  Some comments added
//
//   ----------------------------------------------------------------------
//
//
//----------------------------------------------------------------------


#include "arch_keyboard_manager.h"
#include "kprintf.h"

  /// Static constants
  uint8 const  KeyboardManager::E0_BASE = 96;
  
  uint8 const  KeyboardManager::E0_KPENTER     = 
  KeyboardManager::E0_BASE + 1;
  uint8 const  KeyboardManager::E0_RCTRL       =
  KeyboardManager::E0_BASE + 2;
  uint8 const  KeyboardManager::E0_KPSLASH     =
  KeyboardManager::E0_BASE + 3;
  uint8 const  KeyboardManager::E0_PRSCR       =
  KeyboardManager::E0_BASE + 4;
  uint8 const  KeyboardManager::E0_RALT        =
  KeyboardManager::E0_BASE + 5;
  uint8 const  KeyboardManager::E0_BREAK       =
  KeyboardManager::E0_BASE + 6;
  uint8 const  KeyboardManager::E0_HOME        =
  KeyboardManager::E0_BASE + 7;
  uint8 const  KeyboardManager::E0_UP          =
  KeyboardManager::E0_BASE + 8;
  uint8 const  KeyboardManager::E0_PGUP        =
  KeyboardManager::E0_BASE + 9;
  uint8 const  KeyboardManager::E0_LEFT        =
  KeyboardManager::E0_BASE + 10;
  uint8 const  KeyboardManager::E0_RIGHT       =
  KeyboardManager::E0_BASE + 11;
  uint8 const  KeyboardManager::E0_END         =
  KeyboardManager::E0_BASE + 12;
  uint8 const  KeyboardManager::E0_DOWN        =
  KeyboardManager::E0_BASE + 13;
  uint8 const  KeyboardManager::E0_PGDN        =
  KeyboardManager::E0_BASE + 14;
  uint8 const  KeyboardManager::E0_INS         =
  KeyboardManager::E0_BASE + 15;
  uint8 const  KeyboardManager::E0_MACRO       =
  KeyboardManager::E0_BASE + 16;
  uint8 const  KeyboardManager::E0_F13 = 
  KeyboardManager::E0_BASE + 17;
  uint8 const  KeyboardManager::E0_F14 =
  KeyboardManager::E0_BASE + 18;
  uint8 const  KeyboardManager::E0_HELP =
  KeyboardManager::E0_BASE + 19;
  uint8 const  KeyboardManager::E0_DO =
  KeyboardManager::E0_BASE + 20;
  uint8 const  KeyboardManager::E0_F17 =
  KeyboardManager::E0_BASE + 21;
  uint8 const  KeyboardManager::E0_KPMINPLUS =
  KeyboardManager::E0_BASE + 22;
  uint8 const  KeyboardManager::E1_PAUSE =
  KeyboardManager::E0_BASE + 23;
  
  uint32 const KeyboardManager::KEY_MAPPING_SIZE = 0x80;
  
  uint32 const KeyboardManager::KBD_META_LALT  = 0x0200;
  uint32 const KeyboardManager::KBD_META_RALT  = 0x0400;
  uint32 const KeyboardManager::KBD_META_CTRL  = 0x0800;
  uint32 const KeyboardManager::KBD_META_SHIFT = 0x1000;
  uint32 const KeyboardManager::KBD_META_CAPS  = 0x2000;
  uint32 const KeyboardManager::KBD_META_NUM   = 0x4000;
  uint32 const KeyboardManager::KBD_META_SCRL  = 0x8000;
  
  uint8 const KeyboardManager::KEY_F1    = 0x80;
  uint8 const KeyboardManager::KEY_F2    = (KeyboardManager::KEY_F1 + 1);
  uint8 const KeyboardManager::KEY_F3    = (KeyboardManager::KEY_F2 + 1);
  uint8 const KeyboardManager::KEY_F4    = (KeyboardManager::KEY_F3 + 1);
  uint8 const KeyboardManager::KEY_F5    = (KeyboardManager::KEY_F4 + 1);
  uint8 const KeyboardManager::KEY_F6    = (KeyboardManager::KEY_F5 + 1);
  uint8 const KeyboardManager::KEY_F7    = (KeyboardManager::KEY_F6 + 1);
  uint8 const KeyboardManager::KEY_F8    = (KeyboardManager::KEY_F7 + 1);
  uint8 const KeyboardManager::KEY_F9    = (KeyboardManager::KEY_F8 + 1);
  uint8 const KeyboardManager::KEY_F10   = (KeyboardManager::KEY_F9 + 1);
  uint8 const KeyboardManager::KEY_F11   = (KeyboardManager::KEY_F10 + 1);
  uint8 const KeyboardManager::KEY_F12   = (KeyboardManager::KEY_F11 + 1);
  
  uint8 const KeyboardManager::KEY_INS   = 0x90;
  uint8 const KeyboardManager::KEY_DEL   = (KeyboardManager::KEY_INS + 1);
  uint8 const KeyboardManager::KEY_HOME  = (KeyboardManager::KEY_DEL + 1);
  uint8 const KeyboardManager::KEY_END   = (KeyboardManager::KEY_HOME + 1);
  uint8 const KeyboardManager::KEY_PGUP  = (KeyboardManager::KEY_END + 1);
  uint8 const KeyboardManager::KEY_PGDN  = (KeyboardManager::KEY_PGUP + 1);
  uint8 const KeyboardManager::KEY_LFT   = (KeyboardManager::KEY_PGDN + 1);
  uint8 const KeyboardManager::KEY_UP    = (KeyboardManager::KEY_LFT + 1);
  uint8 const KeyboardManager::KEY_DN    = (KeyboardManager::KEY_UP + 1);
  uint8 const KeyboardManager::KEY_RT    = (KeyboardManager::KEY_DN + 1);
  uint8 const KeyboardManager::KEY_PRNT  = (KeyboardManager::KEY_RT + 1);
  uint8 const KeyboardManager::KEY_PAUSE = (KeyboardManager::KEY_PRNT + 1);
  uint8 const KeyboardManager::KEY_LWIN  = (KeyboardManager::KEY_PAUSE + 1);
  uint8 const KeyboardManager::KEY_RWIN   = (KeyboardManager::KEY_LWIN + 1);
  uint8 const KeyboardManager::KEY_MENU  = (KeyboardManager::KEY_RWIN + 1);
  
  uint32 const KeyboardManager::STANDARD_KEYMAP[KeyboardManager::KEY_MAPPING_SIZE] =
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

  uint32 const KeyboardManager::E0_KEYS[KeyboardManager::KEY_MAPPING_SIZE] =
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
      
KeyboardManager * KeyboardManager::instance_ = 0;

KeyboardManager::KeyboardManager() : extended_scancode( 0 ), keyboard_status_ ( 0 )
{
  keyboard_buffer_ = new RingBuffer<uint8>( 256 ); 
}

KeyboardManager::~KeyboardManager()
{
  delete keyboard_buffer_;
}

void KeyboardManager::kb_wait()
{
  uint32 i;
  
  for(i=0; i<0x10000; i++)
  {
    uint8 stat = inportb(0x64);
    if((stat & 0x02) == 0)
      break;
  }
  if (i>=0x10000)
    kprintfd_nosleep("KeyboardManager::kb_wait: waitiong on 0x02 didn't speed up things :-(\n");
}

void KeyboardManager::send_cmd( uint8 cmd, uint8 port = 0x64 )
{
  kb_wait();
  outbp( port, cmd );
}

void KeyboardManager::serviceIRQ( void )
{
  send_cmd(0xAD);      // disable the keyboard
  kb_wait();

  uint8 scancode = inportb( 0x60 );
  
  if( extended_scancode == 0xE0 )
  {
    if( scancode == 0x2A || scancode == 0x36 || scancode >= E0_BASE )
    {
      extended_scancode = 0;
      send_cmd(0xAE);  // enable the keyboard
      return;
    }
      
    scancode = E0_KEYS[ scancode ];
  }
  else if ( extended_scancode == 0xE1 && scancode == 0x1D )
  {
    extended_scancode = 0x100;  
    send_cmd(0xAE);  // enable the keyboard
    return;  
  }
  else if ( extended_scancode == 0x100 && scancode == 0x45 )
    scancode = E1_PAUSE;
  
  extended_scancode = 0;
    
  if( scancode == 0xFF || scancode == 0xFA 
  || scancode == 0xFE || scancode ==0x00 ) // non parsable codes, ACK and keyb. buffer errors
  {
    kprintfd( "Non-parsable scancode %X \n", scancode );
    send_cmd(0xAE);  // enable the keyboard
    return;
  }

  if( scancode == 0xE0 || scancode == 0xE1 )
  {
    extended_scancode = scancode;
    send_cmd(0xAE);  // enable the keyboard    
    return;
  }

  modifyKeyboardStatus( scancode ); // handle num, caps, scroll, shift, ctrl and alt   
  setLEDs();         // setting the leds
      
  if( (scancode & 0200 ) ) // if a key was released just ignore it
  {
    send_cmd(0xAE);  // enable the keyboard    
    return;
  }
      
  keyboard_buffer_->put( scancode ); // put it inside the buffer
  
  send_cmd(0xAE);    // enable the keyboard 
  
}

void KeyboardManager::modifyKeyboardStatus(uint8 sc )
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

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

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
    send_cmd( 0xF4, 0x60 );  // enable keyboard command
    send_cmd( 0xED, 0x60 );  // "set LEDs" command
    send_cmd( leds, 0x60 );  // bottom 3 bits set LEDs
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
  if (keyboard_buffer_->get(sc))
  {
    key = convertScancode(sc);
    return true;
  }
  else 
    return false;
}
