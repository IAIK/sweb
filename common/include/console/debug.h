#pragma once

#include "kprintf.h"

enum AnsiColor
{
  Ansi_Red = 31,
  Ansi_Green = 32,
  Ansi_Yellow = 33,
  Ansi_Blue = 34,
  Ansi_Magenta = 35,
  Ansi_Cyan = 36,
  Ansi_White = 37,
};


#define DEBUG_COLOR 1
#define DEBUG_TO_FB 0


#define __STRINGIFY2(x) #x
#define __STRINGIFY(x) __STRINGIFY2(x)
#define DEBUG_STR_HERE2(file, line)  " at " file ":" __STRINGIFY(line)
#define DEBUG_STR_HERE DEBUG_STR_HERE2(__FILE__, __LINE__)

#define OUTPUT_ENABLED  0x80000000
#define OUTPUT_ADVANCED 0x40000000
#define OUTPUT_FLAGS (OUTPUT_ENABLED | OUTPUT_ADVANCED)

#define DEBUG_FORMAT_STRING_COLOR   "\033[1;%zum[%-11s]\033[0;39m"
#define DEBUG_FORMAT_STRING_NOCOLOR "[%-11s]"

#define FLAG_PARAM_COLOR(flag) (flag & ~OUTPUT_FLAGS), #flag
#define FLAG_PARAM_NOCOLOR(flag) #flag

#if DEBUG_COLOR
#define DEBUG_FORMAT_STRING DEBUG_FORMAT_STRING_COLOR
#define FLAG_PARAM FLAG_PARAM_COLOR
#else
#define DEBUG_FORMAT_STRING DEBUG_FORMAT_STRING_NOCOLOR
#define FLAG_PARAM FLAG_PARAM_NOCOLOR
#endif

extern bool debug_print_to_fb;

#ifndef EXE2MINIXFS
#define debug(flag, ...) do {                                                                       \
          if (flag & OUTPUT_ENABLED) {                                                              \
            kprintfd(DEBUG_FORMAT_STRING, FLAG_PARAM(flag)); kprintfd(__VA_ARGS__);                 \
            if(debug_print_to_fb) {                                                                 \
              kprintf(DEBUG_FORMAT_STRING_NOCOLOR, FLAG_PARAM_NOCOLOR(flag)); kprintf(__VA_ARGS__); \
            }                                                                                       \
          } } while (0)
#endif





//group Console
const size_t KPRINTF            = Ansi_Yellow  | OUTPUT_ENABLED;
const size_t CONSOLE            = Ansi_Yellow  | OUTPUT_ENABLED;
const size_t TERMINAL           = Ansi_Yellow  | OUTPUT_ENABLED;

//group kernel
const size_t LOCK               = Ansi_Yellow;
const size_t LOADER             = Ansi_White   | OUTPUT_ENABLED;
const size_t SCHEDULER          = Ansi_Yellow;
const size_t SCHEDULER_LOCK     = Ansi_Red;
const size_t SYSCALL            = Ansi_Blue    | OUTPUT_ENABLED;
const size_t MAIN               = Ansi_Red     | OUTPUT_ENABLED;
const size_t THREAD             = Ansi_Magenta | OUTPUT_ENABLED;
const size_t USERPROCESS        = Ansi_Cyan    | OUTPUT_ENABLED;
const size_t PROCESS_REG        = Ansi_Yellow  | OUTPUT_ENABLED;
const size_t BACKTRACE          = Ansi_Red     | OUTPUT_ENABLED;
const size_t USERTRACE          = Ansi_Red     | OUTPUT_ENABLED;

//group memory management
const size_t PM                 = Ansi_Green   | OUTPUT_ENABLED;
const size_t PAGEFAULT          = Ansi_Green   | OUTPUT_ENABLED;
const size_t CPU_ERROR          = Ansi_Red     | OUTPUT_ENABLED;
const size_t KMM                = Ansi_Yellow;

//group driver
const size_t DRIVER             = Ansi_Yellow;
const size_t ATA_DRIVER         = Ansi_Yellow;
const size_t IDE_DRIVER         = Ansi_Yellow;
const size_t MMC_DRIVER         = Ansi_Yellow;
const size_t RAMDISK            = Ansi_Yellow;

//group Block Device
const size_t BD_MANAGER         = Ansi_Yellow;
const size_t BD_VIRT_DEVICE     = Ansi_Yellow;

//group arch
const size_t A_BOOT             = Ansi_Yellow  | OUTPUT_ENABLED;
const size_t A_COMMON           = Ansi_Yellow  | OUTPUT_ENABLED;
const size_t A_MEMORY           = Ansi_Yellow;
const size_t A_SERIALPORT       = Ansi_Yellow;
const size_t A_KB_MANAGER       = Ansi_Yellow;
const size_t A_INTERRUPTS       = Ansi_Yellow;
const size_t A_MULTICORE        = Ansi_Yellow  | OUTPUT_ENABLED;

const size_t ACPI               = Ansi_Red     | OUTPUT_ENABLED;
const size_t APIC               = Ansi_Yellow  | OUTPUT_ENABLED;

//group file system
const size_t FS                 = Ansi_Yellow;
const size_t RAMFS              = Ansi_White;
const size_t DENTRY             = Ansi_Blue;
const size_t PATHWALKER         = Ansi_Yellow;
const size_t PSEUDOFS           = Ansi_Yellow;
const size_t VFSSYSCALL         = Ansi_Yellow;
const size_t VFS                = Ansi_Yellow;

//group minix
const size_t M_STORAGE_MANAGER  = Ansi_Yellow;
const size_t M_INODE            = Ansi_Yellow;
const size_t M_SB               = Ansi_Yellow;
const size_t M_ZONE             = Ansi_Yellow;;
