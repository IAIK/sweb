/**
 * @file kprintf.cpp
 */

#include "stdarg.h"
#include "kprintf.h"
#include "Console.h"
#include "Terminal.h"
#include "debug_bochs.h"
#include "ArchInterrupts.h"
#include "RingBuffer.h"
#include "Scheduler.h"
#include "assert.h"
#include "debug.h"

//it's more important to keep the messages that led to an error, instead of
//the ones following it, when the nosleep buffer gets full

void oh_writeCharWithSleep ( char c )
{
  main_console->getActiveTerminal()->write ( c );
}
void oh_writeStringWithSleep ( char const* str )
{
  main_console->getActiveTerminal()->writeString ( str );
}

RingBuffer<char> *nosleep_rb_;

void flushActiveConsole()
{
  assert(main_console);
  assert(nosleep_rb_);
  assert ( ArchInterrupts::testIFSet() );
  char c=0;
  while ( nosleep_rb_->get ( c ) )
    main_console->getActiveTerminal()->write ( c );
}

class KprintfNoSleepFlushingThread : public Thread
{
  public:

    KprintfNoSleepFlushingThread() : Thread("KprintfNoSleepFlushingThread")
    {
    }

    virtual void Run()
    {
      while ( true )
      {
        flushActiveConsole();
        Scheduler::instance()->yield();
      }
    }
};

void kprintf_init()
{
  nosleep_rb_ = new RingBuffer<char> ( 1024 );
  debug ( KPRINTF,"Adding Important kprintf Flush Thread\n" );
  Scheduler::instance()->addNewThread ( new KprintfNoSleepFlushingThread() );
}

void oh_writeCharNoSleep ( char c )
{
  nosleep_rb_->put ( c );
}
void oh_writeStringNoSleep ( char const* str )
{
  while ( *str )
  {
    oh_writeCharNoSleep ( *str );
    str++;
  }
}

void oh_writeCharDebugNoSleep ( char c )
{
  writeChar2Bochs ( ( uint8 ) c );
}

void oh_writeStringDebugNoSleep ( char const* str )
{
  size_t i = 251;
  while (*str && --i)
  {
    oh_writeCharDebugNoSleep ( *str );
    str++;
  }
}


uint8 const ZEROPAD = 1;    /* pad with zero */
uint8 const SIGN  = 2;    /* unsigned/signed long */
uint8 const PLUS  = 4;    /* show plus */
uint8 const SPACE = 8;    /* space if plus */
uint8 const LEFT  = 16;   /* left justified */
uint8 const SPECIAL = 32;   /* 0x */
uint8 const LARGE = 64;   /* use 'ABCDEF' instead of 'abcdef' */

void output_number_long(void (*write_char)(char), uint64 num, uint64 base,
                        size_t size, size_t precision, uint8 type)
{
  char c;
  char sign, tmp[70];
  const char* digits;
  static const char small_digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
  static const char large_digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  size_t i;

  digits = (type & LARGE) ? large_digits : small_digits;
  if (type & LEFT)
  {
    type &= ~ZEROPAD;
    size = 0;  // no padding then
  }
  if (base < 2 || base > 36)
    return;
  c = (type & ZEROPAD) ? '0' : ' ';
  sign = 0;
  if (type & SIGN)
  {
    if (((int64)num) < 0)
    {
      sign = '-';
      num = -(int64)num;
    }
    else if (type & PLUS)
    {
      sign = '+';
    }
    else if (type & SPACE)
    {
      sign = ' ';
    }
  }
  i = 0;
  if (num == 0)
    tmp[i++] = '0';
  else
    while (num != 0)
    {
      uint64 index = num % base;
      tmp[i++] = digits[index];
      num /= base;
    }
  //    size -= precision;
  //    if (!(type&(ZEROPAD+LEFT))) {
  //      while(size-- >0) {
  //        console->write(' ');
  //      }
  //    }
  if (sign)
  {
    tmp[i++] = sign;
  }
  if (type & SPECIAL)
  {
    precision = 0;  // no precision with special for now
    if (base == 8)
    {
      tmp[i++] = '0';
    }
    else if (base == 16)
    {
      tmp[i++] = digits[33];
      tmp[i++] = '0';
    }
  }

  if (precision > size)
    precision = size;

  while (size-- - precision > i)
    (write_char)((char)c);

  while (precision-- > i)
    (write_char)((char)'0');

  while (i-- > 0)
    (write_char)((char)tmp[i]);
  //   while (size-- > 0) {
  //   if (buf <= end)
  //   *buf = ' ';
  //   ++buf;
  //   }
}

void output_number ( void ( *write_char ) ( char ), size_t num, size_t base, size_t size, size_t precision, uint8 type )
{
  char c;
  char sign,tmp[70];
  const char *digits;
  static const char small_digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
  static const char large_digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  size_t i;

  digits = ( type & LARGE ) ? large_digits : small_digits;
  if ( type & LEFT )
  {
    type &= ~ZEROPAD;
    size = 0;  //no padding then
  }
  if ( base < 2 || base > 36 )
    return;
  c = ( type & ZEROPAD ) ? '0' : ' ';
  sign = 0;
  if ( type & SIGN )
  {
    if ( ( ( ssize_t ) num ) < 0 )
    {
      sign = '-';
      num = - ( ssize_t ) num;
    }
    else if ( type & PLUS )
    {
      sign = '+';
    }
    else if ( type & SPACE )
    {
      sign = ' ';
    }
  }
  i = 0;
  if ( num == 0 )
    tmp[i++]='0';
  else while ( num != 0 )
    {
      tmp[i++] = digits[num%base];
      num /= base;
    }
//    size -= precision;
//    if (!(type&(ZEROPAD+LEFT))) {
//      while(size-- >0) {
//        console->write(' ');
//      }
//    }
  if ( sign )
  {
    tmp[i++] = sign;
  }
  if ( type & SPECIAL )
  {
    precision = 0; //no precision with special for now
    if ( base==8 )
    {
      tmp[i++] = '0';
    }
    else if ( base==16 )
    {
      tmp[i++] = digits[33];
      tmp[i++] = '0';
    }
  }

  if ( precision > size )
    precision = size;

  while ( size-- - precision > i )
    ( write_char ) ( ( char ) c );

  while ( precision-- > i )
    ( write_char ) ( ( char ) '0' );

  while ( i-- > 0 )
    ( write_char ) ( ( char ) tmp[i] );
//   while (size-- > 0) {
//   if (buf <= end)
//   *buf = ' ';
//   ++buf;
//   }
}

size_t atoi ( const char *&fmt )
{
  size_t num=0;
  while ( *fmt >= '0' && *fmt <= '9' )
  {
    num*= 10;
    num+=*fmt - '0';
    ++fmt;
  }
  return num;
}

void vkprint_buffer ( void ( *write_char ) ( char ), char *buffer, size_t size )
{
  for ( size_t c=0; c<size; ++c )
    write_char ( ( char ) buffer[c] );
}

// doesn't know flags yet
void vkprintf ( void ( *write_string ) ( char const* ), void ( *write_char ) ( char ), const char *fmt, va_list args )
{
  while ( fmt && *fmt )
  {
    if ( *fmt == '%' )
    {
      ssize_t width = 0;
      uint8 flag = 0;
      char *tmp=0;
      ++fmt;
      switch ( *fmt )
      {
        case '-':
          flag |= LEFT;
          ++fmt;
          break;
        case '+':
          flag |= PLUS;
          ++fmt;
          break;
        case '0':
          flag |= ZEROPAD;
          ++fmt;
          break;
        default:
          break;
      }
      if ( *fmt > '0' && *fmt <= '9' )
        width = atoi ( fmt );  //this advances *fmt as well

      //handle diouxXfeEgGcs
      switch ( *fmt )
      {
        case '%':
          write_char ( *fmt );
          break;

        case 's':
          tmp = ( char* ) va_arg ( args,char const* );
          if ( tmp )
            write_string ( tmp );
          else
            write_string ( "(null)" );
          break;

          //print a Buffer, this expects the buffer size as next argument
          //and is quite non-standard :)
        case 'B':
          tmp = ( char* ) va_arg ( args,char* );
          width = ( size_t ) va_arg ( args,size_t );
          if ( tmp )
            vkprint_buffer ( write_char, tmp, width );
          else
            write_string ( "(null)" );
          break;

          //signed decimal
        case 'd':
          output_number ( write_char, ( size_t ) va_arg ( args,ssize_t ),10,width, 0, flag | SIGN );
          break;

          //we don't do i until I see what it actually should do
          //case 'i':
          //  break;

          //octal
        case 'o':
          output_number ( write_char, ( size_t ) va_arg ( args,size_t ),8,width, 0, flag | SPECIAL );
          break;

          //unsigned
        case 'u':
          output_number ( write_char, ( size_t ) va_arg ( args,size_t ),10,width, 0, flag );
          break;

        case 'x':
          output_number ( write_char, ( size_t ) va_arg ( args,size_t ),16,width, 0, flag | SPECIAL );
          break;

        case 'y':
          output_number ( write_char, ( size_t ) va_arg ( args,size_t ),16,width, 0, flag );
          break;

        case 'X':
          output_number ( write_char, ( size_t ) va_arg ( args,size_t ), 16, width, 0, flag | SPECIAL | LARGE );
          break;

        case 'l':
          switch (*(fmt + 1))
          {
            case 'u':
              output_number_long(write_char, (uint64)va_arg(args, uint64), 10,
                                 width, 0, flag);
              fmt++;
              break;
            case 'x':
              output_number_long(write_char, (uint64)va_arg(args, uint64), 16,
                                 width, 0, flag);
              fmt++;
              break;
            default:
              output_number_long(write_char, (uint64)va_arg(args, uint64), 10,
                                 width, 0, flag | SIGN);
          }
          break;


          //no floating point yet
          //case 'f':
          //  break;

          //no scientific notation (yet)
          //case 'e':
          //  break;

          //case 'E':
          //  break;

          //no floating point yet
          //case 'g':
          //  break;

          //case 'G':
          //  break;

          //we don't do unicode (yet)
        case 'c':
          write_char ( ( char ) va_arg ( args,size_t ) );
          break;

        default:
          //jump over unknown arg
          va_arg(args,size_t);
          break;
      }

    }
    else
      write_char ( *fmt );

    ++fmt;
  }

}

void kprintf ( const char *fmt, ... )
{
  va_list args;

  va_start ( args, fmt );
  //check if atomar or not in current context
  if ( ( ArchInterrupts::testIFSet() && Scheduler::instance()->isSchedulingEnabled() )
          || ( main_console->areLocksFree() && main_console->getActiveTerminal()->isLockFree() ) )
    vkprintf ( oh_writeStringWithSleep, oh_writeCharWithSleep, fmt, args );
  else
    vkprintf ( oh_writeStringNoSleep, oh_writeCharNoSleep, fmt, args );
  va_end ( args );
}

void kprintfd ( const char *fmt, ... )
{
  va_list args;

  va_start ( args, fmt );
  vkprintf ( oh_writeStringDebugNoSleep, oh_writeCharDebugNoSleep, fmt, args );
  va_end ( args );
}

void kprint_buffer ( char *buffer, size_t size )
{
  if ( unlikely ( ArchInterrupts::testIFSet() ) )
    vkprint_buffer ( oh_writeCharWithSleep, buffer, size );
  else
    vkprint_buffer ( oh_writeCharNoSleep, buffer, size );
}

void kprintd_buffer ( char *buffer, size_t size )
{
    vkprint_buffer ( oh_writeCharDebugNoSleep, buffer, size );
}

bool isDebugEnabled ( size_t flag )
{
  bool group_enabled = false;

  if ( ! ( flag & OUTPUT_ENABLED ) )
  {
    size_t group_flag = flag & 0x7fff0000;
    group_flag |= OUTPUT_ENABLED;
    switch ( group_flag )
    {
      case BD:
      case CONSOLE:
      case KERNEL:
      case MM:
      case VFSSYSCALL:
      case DRIVER:
      case ARCH:
        group_enabled = true;
        break;
    }
  }
  if ( ( flag & OUTPUT_ENABLED ) || group_enabled )
  {
    return true;
  }
  return false;
}

#ifndef NO_COLOR
#define COLORDEBUG(str, color) "\033[0;%sm%s\033[1;m", color, str
#else
#define COLORDEBUG(str, color) str
#endif

void debug ( size_t flag, const char *fmt, ... )
{
  va_list args;
  va_start ( args, fmt );
  if ( isDebugEnabled ( flag ) )
  {
    switch ( flag )
    {
      case CACHE:
        kprintfd ( COLORDEBUG("[CACHE      ]", "31"));
        break;
      case READ_CACHE:
        kprintfd ( COLORDEBUG("[READ_CACHE ]", "32"));
        break;
      case WRITE_CACHE:
        kprintfd ( COLORDEBUG("[WRITE_CACHE]", "33"));
        break;
      case BD_MANAGER:
        kprintfd ( COLORDEBUG("[BD_MANAGER ]", "33"));
        break;
      case KPRINTF:
        kprintfd ( COLORDEBUG("[KPRINTF    ]", "33"));
        break;
      case CONDITION:
        kprintfd ( COLORDEBUG("[CONDITION  ]", "33"));
        break;
      case LOADER:
        kprintfd ( COLORDEBUG("[LOADER     ]", "37"));
        break;
      case SCHEDULER:
        kprintfd ( COLORDEBUG("[SCHEDULER  ]", "33"));
        break;
      case SYSCALL:
        kprintfd ( COLORDEBUG("[SYSCALL    ]", "34"));
        break;
      case MAIN:
        kprintfd ( COLORDEBUG("[MAIN       ]", "31"));
        break;
      case THREAD:
        kprintfd ( COLORDEBUG("[THREAD     ]", "35"));
        break;
      case USERPROCESS:
        kprintfd ( COLORDEBUG("[USERPROCESS]", "36"));
        break;
      case MOUNTMINIX:
        kprintfd ( COLORDEBUG("[MOUNTMINIX ]", "36"));
        break;
      case BACKTRACE:
        kprintfd ( COLORDEBUG("[BACKTRACE  ]", "31"));
        break;
      case PM:
        kprintfd ( COLORDEBUG("[PM         ]", "32"));
        break;
      case KMM:
        kprintfd ( COLORDEBUG("[KMM        ]", "33"));
        break;
      case ATA_DRIVER:
        kprintfd ( COLORDEBUG("[ATA_DRIVER ]", "33"));
        break;
      case MMC_DRIVER:
        kprintfd ( COLORDEBUG("[MMC_DRIVER ]", "33"));
        break;
      case IDE_DRIVER:
        kprintfd ( COLORDEBUG("[IDE_DRIVER ]", "33"));
        break;
      case A_COMMON:
        kprintfd ( COLORDEBUG("[A_COMMON   ]", "33"));
        break;
      case A_MEMORY:
        kprintfd ( COLORDEBUG("[A_MEMORY   ]", "33"));
        break;
      case A_SERIALPORT:
        kprintfd ( COLORDEBUG("[A_SERIALPRT]", "33"));
        break;
      case A_KB_MANAGER:
        kprintfd ( COLORDEBUG("[A_KB_MANAGR]", "33"));
        break;
      case BD_VIRT_DEVICE:
        kprintfd ( COLORDEBUG("[BD_VIRT_DEV]", "33"));
        break;
      case A_INTERRUPTS:
        kprintfd ( COLORDEBUG("[A_INTERRUPT]", "33"));
        break;
      case VFSSYSCALL:
        kprintfd ( COLORDEBUG("[VFSSYSCALL ]", "33"));
        break;
      case FILE_SYSTEM:
        kprintfd ( COLORDEBUG("[FILE_SYSTEM]", "32"));
        break;
      case VOLUME_MANAGER:
        kprintfd ( COLORDEBUG("[VOLUME-MAN ]", "31"));
        break;
      case FS_DEVICE:
        kprintfd ( COLORDEBUG("[FS_DEVICE  ]", "35"));
        break;
      case FS_BITMAP:
        kprintfd ( COLORDEBUG("[FS_BITMAP  ]", "36"));
        break;
      case FS_INODE:
        kprintfd ( COLORDEBUG("[INODE      ]", "38"));
        break;
      case FS_UTIL:
        kprintfd ( COLORDEBUG("[FS_UTIL    ]", "39"));
        break;
      case FS_UNIX:
        kprintfd ( COLORDEBUG("[FS_UNIX    ]", "32"));
        break;
      case INODE_TABLE:
        kprintfd ( COLORDEBUG("[INODETABLE ]", "37"));
        break;
      case FS_MINIX:
        kprintfd ( COLORDEBUG("[FS_MINIX   ]", "32"));
        break;
    }
    vkprintf ( oh_writeStringDebugNoSleep, oh_writeCharDebugNoSleep, fmt, args );
  }

  va_end ( args );
}
