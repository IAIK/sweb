/**
 * @file KeyboardManager.h
 *
 */

#ifndef _KEYBOARD_MANAGER_H
#define _KEYBOARD_MANAGER_H

#ifdef __cplusplus

extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

#include "RingBuffer.h"
#include "atkbd.h"

#define E0_BASE      96
#define E0_KPENTER   (E0_BASE +  1)
#define E0_RCTRL     (E0_BASE +  2)
#define E0_KPSLASH   (E0_BASE +  3)
#define E0_PRSCR     (E0_BASE +  4)
#define E0_RALT      (E0_BASE +  5)
#define E0_BREAK     (E0_BASE +  6)
#define E0_HOME      (E0_BASE +  7)
#define E0_UP        (E0_BASE +  8)
#define E0_PGUP      (E0_BASE +  9)
#define E0_LEFT      (E0_BASE + 10)
#define E0_RIGHT     (E0_BASE + 11)
#define E0_END       (E0_BASE + 12)
#define E0_DOWN      (E0_BASE + 13)
#define E0_PGDN      (E0_BASE + 14)
#define E0_INS       (E0_BASE + 15)
#define E0_MACRO     (E0_BASE + 16)
#define E0_F13       (E0_BASE + 17)
#define E0_F14       (E0_BASE + 18)
#define E0_HELP      (E0_BASE + 19)
#define E0_DO        (E0_BASE + 20)
#define E0_F17       (E0_BASE + 21)
#define E0_KPMINPLUS (E0_BASE + 22)
#define E1_PAUSE     (E0_BASE + 23)

#define KEY_MAPPING_SIZE 0x80

#define KBD_META_LALT  0x0200
#define KBD_META_RALT  0x0400
#define KBD_META_CTRL  0x0800
#define KBD_META_SHIFT 0x1000
#define KBD_META_CAPS  0x2000
#define KBD_META_NUM   0x4000
#define KBD_META_SCRL  0x8000

#define KEY_F1    0x80
#define KEY_F2    (KEY_F1 + 2)
#define KEY_F3    (KEY_F1 + 3)
#define KEY_F4    (KEY_F1 + 4)
#define KEY_F5    (KEY_F1 + 5)
#define KEY_F6    (KEY_F1 + 6)
#define KEY_F7    (KEY_F1 + 7)
#define KEY_F8    (KEY_F1 + 8)
#define KEY_F9    (KEY_F1 + 9)
#define KEY_F10   (KEY_F1 + 10)
#define KEY_F11   (KEY_F1 + 11)
#define KEY_F12   (KEY_F1 + 12)

#define KEY_INS    0x90
#define KEY_DEL    (KEY_INS +  1)
#define KEY_HOME   (KEY_INS +  2)
#define KEY_END    (KEY_INS +  3)
#define KEY_PGUP   (KEY_INS +  4)
#define KEY_PGDN   (KEY_INS +  5)
#define KEY_LFT    (KEY_INS +  6)
#define KEY_UP     (KEY_INS +  7)
#define KEY_DN     (KEY_INS +  8)
#define KEY_RT     (KEY_INS +  9)
#define KEY_PRNT   (KEY_INS + 10)
#define KEY_PAUSE  (KEY_INS + 11)
#define KEY_LWIN   (KEY_INS + 12)
#define KEY_RWIN   (KEY_INS + 13)
#define KEY_MENU   (KEY_INS + 14)

/** @class KeyboardManager
 *
 * Class that contains architecture specific parameters for AT keyboard
 * communication
 *
 */
class KeyboardManager
{
  public:
    KeyboardManager();
    ~KeyboardManager();
    static KeyboardManager *instance()
    {
      if (!instance_)
        instance_ = new KeyboardManager();
      return instance_;
    }
    ;

    bool getKeyFromKbd(uint32 &key);
    void serviceIRQ(void);

    bool isShift();
    bool isCtrl();
    bool isAlt();
    bool isAltGr();
    bool isCaps();
    bool isNum();
    bool isScroll();

    void emptyKbdBuffer();

  private:

    /**
     * function is called when the Keyboard has to wait
     *
     */
    void kb_wait();

    /**
     * writes a byte to the given IO port
     *
     */
    void send_cmd(uint8 cmd, uint8 port = 0);

    RingBuffer<uint8> keyboard_buffer_;

    static uint32 const STANDARD_KEYMAP[];
    static uint32 const E0_KEYS[];

    /**
     * converts the scancode into a key by looking in the Standard KeyMap
     *
     */
    uint32 convertScancode(uint8 scancode);

    /**
     * function is called to handle num, caps, scroll, shift, ctrl and alt
     *
     */
    void modifyKeyboardStatus(uint8 sc);

    void setLEDs(void);

    uint32 extended_scancode;
    uint32 keyboard_status_;
    uint32 usb_kbd_addr_;
    uint32 current_key_;

  protected:

    static KeyboardManager *instance_;
};

#endif
