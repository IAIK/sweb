//----------------------------------------------------------------------
//   $Id: kprintf.cpp,v 1.25 2006/11/24 20:47:53 btittelbach Exp $
//----------------------------------------------------------------------
//   $Log: kprintf.cpp,v $
//   Revision 1.24  2006/10/20 15:13:41  btittelbach
//   kprintfd und kprintfd_nosleep machen jetzt seit einem Jahr dasselbe,
//   jetzt sieht man das im Code wenigstens auch gleich
//
//   Revision 1.23  2005/10/27 21:42:51  btittelbach
//   -Mutex::isFree() abuse check kennt jetzt auch Scheduler-Ausschalten und springt nicht mehr versehentlich an
//   -im Scheduler möglichen null-pointer zugriff vermieden
//   -kprintf nosleep check logik optimiert
//
//   Revision 1.22  2005/10/26 11:17:40  btittelbach
//   -fixed KMM/SchedulerBlock Deadlock
//   -introduced possible dangeours reenable-/disable-Scheduler Methods
//   -discovered that removing the IF/Lock Checks in kprintfd_nosleep is a VERY BAD Idea
//
//   Revision 1.21  2005/10/26 11:07:32  btittelbach
//   sorry, it's not that easy...
//   kprintf_nosleep MUST Check for Interrupts and Scheduler, otherwise
//   the kprintf_nosleep ringbuffer won't be locked and can get corrupted
//
//   Revision 1.20  2005/10/26 10:09:17  btittelbach
//   kprintf(d)_nosleep Functions now _always_ use the nosleep handlers,
//   since with Scheduler Blocking we cannot be sure that thread switches can happen
//   even if interrupts are on
//
//   Revision 1.19  2005/09/20 08:05:08  btittelbach
//   +kprintf flush fix: even though it worked fine before, now it works fine in theory as well ;->
//   +Condition cleanup
//   +FiFoDRBOSS now obsolete and removed
//   +added disk.img that nelle forgot to check in
//
//   Revision 1.18  2005/09/16 15:47:41  btittelbach
//   +even more KeyboardInput Bugfixes
//   +intruducing: kprint_buffer(..) (console write should never be used directly from anything with IF=0)
//   +Thread now remembers its Terminal
//   +Syscalls are USEABLE !! :-) IF=1 !!
//   +Syscalls can block now ! ;-) Waiting for Input...
//   +more other Bugfixes
//
//   Revision 1.17  2005/09/13 15:00:51  btittelbach
//   Prepare to be Synchronised...
//   kprintf_nosleep works now
//   scheduler/list still needs to be fixed
//
//   Revision 1.16  2005/09/07 00:33:52  btittelbach
//   +More Bugfixes
//   +Character Queue (FiFoDRBOSS) from irq with Synchronisation that actually works
//
//   Revision 1.15  2005/09/03 21:54:45  btittelbach
//   Syscall Testprogramm, actually works now ;-) ;-)
//   Test get autocompiled and autoincluded into kernel
//   one kprintfd bug fixed
//
//   Revision 1.14  2005/08/19 21:14:15  btittelbach
//   Debugging the Debugging Code
//
//   Revision 1.12  2005/08/04 20:47:43  btittelbach
//   Where is the Bug, maybe I will see something tomorrow that I didn't see today
//
//   Revision 1.11  2005/08/04 17:49:22  btittelbach
//   Improved (documented) arch_PageFaultHandler
//   Solution to Userspace Bug still missing....
//
//   Revision 1.10  2005/07/27 13:43:47  btittelbach
//   Interrupt On/Off Autodetection in Kprintf
//
//   Revision 1.9  2005/07/27 10:04:26  btittelbach
//   kprintf_nosleep and kprintfd_nosleep now works
//   Output happens in dedicated Thread using VERY EVIL Mutex Hack
//
//   Revision 1.8  2005/07/24 17:02:59  nomenquis
//   lots of changes for new console stuff
//
//   Revision 1.7  2005/07/05 16:15:48  btittelbach
//   kprintf.cpp
//
//   Revision 1.6  2005/06/05 07:59:35  nelles
//   The kprintf_debug or kprintfd are finished
//
//   Revision 1.5  2005/05/10 17:03:55  btittelbach
//   Kprintf Vorbereitung für Print auf Bochs Console
//   böse .o im source gelöscht
//
//   Revision 1.4  2005/04/26 21:38:43  btittelbach
//   Fifo/Pipe Template soweit das ohne Lock und CV zu implementiern ging
//   kprintf kennt jetzt auch chars
//
//   Revision 1.3  2005/04/24 20:31:33  btittelbach
//   kprintf now knows how to padd
//
//   Revision 1.2  2005/04/24 18:58:45  btittelbach
//   kprintf bugfix
//
//   Revision 1.1  2005/04/24 13:33:40  btittelbach
//   skeleton of a kprintf
//
//
//----------------------------------------------------------------------



#include "stdarg.h"
#include "kprintf.h"
#include "Console.h"
#include "Terminal.h"
#include "debug_bochs.h"
#include "ArchInterrupts.h"
#include "ipc/RingBuffer.h"
#include "Scheduler.h"
#include "assert.h"

//it's more important to keep the messages that led to an error, instead of
//the ones following it, when the nosleep buffer gets full

void oh_writeCharWithSleep(char c)
{
  main_console->getActiveTerminal()->write(c);
}
void oh_writeStringWithSleep(char const* str)
{
  main_console->getActiveTerminal()->writeString(str);
}


// void oh_writeCharDebugWithSleep(char c)
// {
// }
// void oh_writeStringDebugWithSleep(char const* str)
// {
// }

RingBuffer<char> *nosleep_rb_;

void flushActiveConsole()
{
  assert(ArchInterrupts::testIFSet());
  char c=0;
  while (nosleep_rb_->get(c))
    main_console->getActiveTerminal()->write(c);
}

class KprintfNoSleepFlushingThread : public Thread
{
  public:

   KprintfNoSleepFlushingThread()
  {
    name_="KprintfNoSleepFlushingThread";
  }
  
  virtual void Run()
  {
    while (true)
    {
      flushActiveConsole();
      Scheduler::instance()->yield();
    }
  }
};

void kprintf_nosleep_init()
{
  nosleep_rb_ = new RingBuffer<char>(10240);
  kprintf("Adding Important kprintf_nosleep Flush Thread\n");
  Scheduler::instance()->addNewThread(new KprintfNoSleepFlushingThread());
}

void oh_writeCharNoSleep(char c)
{
  nosleep_rb_->put(c);
}
void oh_writeStringNoSleep(char const* str)
{
  while (*str)
  {
    oh_writeCharNoSleep(*str);
    str++;
  }
}

void oh_writeCharDebugNoSleep(char c)
{
  //this blocks
  writeChar2Bochs((uint8) c);
}
void oh_writeStringDebugNoSleep(char const* str)
{
  //this blocks
  while (*str)
  {
    oh_writeCharDebugNoSleep(*str);
    str++;
  }
}


uint8 const ZEROPAD	= 1;		/* pad with zero */
uint8 const SIGN	= 2;		/* unsigned/signed long */
uint8 const PLUS	= 4;		/* show plus */
uint8 const SPACE	= 8;		/* space if plus */
uint8 const LEFT	= 16;		/* left justified */
uint8 const SPECIAL	= 32;		/* 0x */
uint8 const LARGE	= 64;		/* use 'ABCDEF' instead of 'abcdef' */

void output_number(void (*write_char)(char), uint32 num, uint32 base, uint32 size, uint32 precision, uint8 type)
{
	char c;
  char sign,tmp[70];
	const char *digits;
	static const char small_digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
	static const char large_digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	uint32 i;

	digits = (type & LARGE) ? large_digits : small_digits;
	if (type & LEFT)
  {
		type &= ~ZEROPAD;
    size = 0;  //no padding then
  }
	if (base < 2 || base > 36)
		return;
	c = (type & ZEROPAD) ? '0' : ' ';
	sign = 0;
	if (type & SIGN) {
		if (((int32) num) < 0) {
			sign = '-';
			num = - (int32) num;
		} else if (type & PLUS) {
			sign = '+';
		} else if (type & SPACE) {
			sign = ' ';
		}
	}
	i = 0;
	if (num == 0)
		tmp[i++]='0';
	else while (num != 0)
  {
		tmp[i++] = digits[num%base];
    num /= base;
  }
	//size -= precision;
	//~ if (!(type&(ZEROPAD+LEFT))) {
		//~ while(size-- >0) {
      //~ console->write(' ');
    //~ }
	//~ }
	if (sign) {
    tmp[i++] = sign;
  }
	if (type & SPECIAL) {
    precision = 0; //no precision with special for now
		if (base==8) {
        tmp[i++] = '0';
		} else if (base==16) {
        tmp[i++] = digits[33];
        tmp[i++] = '0';
      }
	}

  if (precision > size)
    precision = size;
  
  while (size-- - precision > i)
    (write_char)((char) c);
  
  while (precision-- > i)         
    (write_char)((char) '0');
    
	while (i-- > 0)
    (write_char)((char) tmp[i]);
	//~ while (size-- > 0) {
		//~ if (buf <= end)
			//~ *buf = ' ';
		//~ ++buf;
	//~ }
}

uint32 atoi(const char *&fmt)
{
  uint32 num=0;
  while (*fmt >= '0' && *fmt <= '9')
  {
    num*= 10;
    num+=*fmt - '0';
    ++fmt;
  }
  return num;
}

void vkprint_buffer(void (*write_char)(char), char *buffer, uint32 size)
{
  for (uint32 c=0; c<size; ++c)
    write_char((char) buffer[c]);
}

// simple vkprintf, doesn't know flags yet
// by Bernhard
void vkprintf(void (*write_string)(char const*), void (*write_char)(char), const char *fmt, va_list args)
{  
  while (fmt && *fmt)
  {
    if (*fmt == '%')
    {
      int32 width = 0;
      uint8 flag = 0;
      char *tmp=0;
      ++fmt;
      switch (*fmt) 
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
      if (*fmt > '0' && *fmt <= '9')
        width = atoi(fmt);  //this advances *fmt as well
      
      //handle diouxXfeEgGcs
      switch (*fmt)
      {
        case '%':
          write_char(*fmt);
          break;
        
        case 's':
	  tmp = (char*) va_arg(args,char const*);
	  if (tmp)
	    write_string(tmp);
	  else
	    write_string("(null)");
          break;
        
        //print a Buffer, this expects the buffer size as next argument
        //and is quite non-standard :)
        case 'B':
          tmp = (char*) va_arg(args,char*);
          width = (uint32) va_arg(args,uint32);
	  if (tmp)
            vkprint_buffer(write_char, tmp, width);
	  else
	    write_string("(null)");
          break;
        
        //signed decimal
        case 'd':
          output_number(write_char,(uint32) va_arg(args,int32),10,width, 0, flag | SIGN);
          break;
        
        //we don't do i until I see what it actually should do
        //case 'i':
        //  break;

        //octal
        case 'o':
          output_number(write_char,(uint32) va_arg(args,uint32),8,width, 0, flag | SPECIAL);
          break;

        //unsigned
        case 'u':
          output_number(write_char,(uint32) va_arg(args,uint32),10,width, 0, flag );
          break;

        case 'x':
          output_number(write_char,(uint32) va_arg(args,uint32),16,width, 0, flag | SPECIAL);
          break;

        case 'X':
          output_number(write_char,(uint32) va_arg(args,uint32), 16, width, 0, flag | SPECIAL | LARGE);
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
          write_char((char) va_arg(args,uint32));
          break;

        default:
          //jump over unknown arg
          ++args;
          break;
      }
      
    }
    else
      write_char(*fmt);
    
    ++fmt;
  }
  
}

//------------------------------------------------
/// Standard kprintf. Usable like any other printf. 
/// Outputs Text on the Console. It is intendet for
/// Kernel Debugging andt herefore avoids using "new".
///
/// @return void
/// @param fmt Format String with standard Format Syntax
/// @param args Possible multibple variable for printf as specified in Format String.
///
///
///
void kprintf(const char *fmt, ...)
{
  va_list args;
  
  va_start(args, fmt);
  //check if atomar or not in current context
  if ((ArchInterrupts::testIFSet() && Scheduler::instance()->isSchedulingEnabled())
  || (main_console->areLocksFree() && main_console->getActiveTerminal()->isLockFree()))
    vkprintf(oh_writeStringWithSleep, oh_writeCharWithSleep, fmt, args);
  else
    vkprintf(oh_writeStringNoSleep, oh_writeCharNoSleep, fmt, args);
  va_end(args);
}

//------------------------------------------------
/// kprintfd is a shorthand for kprintf_debug
/// Outputs Text on the Serial Debug Console. It is intendet for
/// Kernel Debugging andt herefore avoids using "new".
///
/// @return void
/// @param fmt Format String with standard Format Syntax
/// @param args Possible multibple variable for printf as specified in Format String.
///
///
///
void kprintfd(const char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  // for a long time now, there was no difference between kprintd and kprintfd_nosleep
  // now it's also immediately obvious
  vkprintf(oh_writeStringDebugNoSleep, oh_writeCharDebugNoSleep, fmt, args);
  va_end(args);
}

//make this obsolete with atomarity check
void kprintf_nosleep(const char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  //check if atomar or not in current context
  //ALSO: this is a kind of lock on the kprintf_nosleep Datastructure, and therefore very important !
  if ((ArchInterrupts::testIFSet() && Scheduler::instance()->isSchedulingEnabled())
  || (main_console->areLocksFree() && main_console->getActiveTerminal()->isLockFree()))
    vkprintf(oh_writeStringWithSleep, oh_writeCharWithSleep, fmt, args);
  else
    vkprintf(oh_writeStringNoSleep, oh_writeCharNoSleep, fmt, args);
  va_end(args);
}
//make this obsolete with atomarity check
void kprintfd_nosleep(const char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  vkprintf(oh_writeStringDebugNoSleep, oh_writeCharDebugNoSleep, fmt, args);
  va_end(args);  
}

void kprint_buffer(char *buffer, uint32 size)
{
  if (unlikely(ArchInterrupts::testIFSet()))
    vkprint_buffer(oh_writeCharWithSleep, buffer, size);
  else
    vkprint_buffer(oh_writeCharNoSleep, buffer, size);
}

void debug(uint32 flag, const char *fmt, ...)
{  
  va_list args;
  va_start(args, fmt);
  
  bool group_enabled = false;
  
  if(!(flag & OUTPUT_ENABLED))
  {
    uint32 group_flag = flag & 0x01110000;
    group_flag |= OUTPUT_ENABLED;
    switch(group_flag)
    {
      case MINIX:
        group_enabled = true;
        break;
        
    }
  }
  
  if((flag & OUTPUT_ENABLED) || group_enabled)
  {
      switch(flag)
      {
        case M_INODE:
          kprintfd("M_INODE:>> ");
          vkprintf(oh_writeStringDebugNoSleep, oh_writeCharDebugNoSleep, fmt, args);
          break;
        case M_STORAGE_MANAGER:
          kprintfd("M_STORAGE_MANAGER:>> ");
          vkprintf(oh_writeStringDebugNoSleep, oh_writeCharDebugNoSleep, fmt, args);
          break;
      }
  }
  
  va_end(args);
}
















