#include "ArchCommon.h"
#include "ArchBoardSpecific.h"
#include "offsets.h"
#include "kprintf.h"
#include "ArchMemory.h"
#include "TextConsole.h"
#include "FrameBufferConsole.h"
#include "backtrace.h"
#include "Stabs2DebugInfo.h"

#define PHYSICAL_MEMORY_AVAILABLE 8*1024*1024

extern void* kernel_end_address;

pointer ArchCommon::getKernelEndAddress()
{
   return (pointer)&kernel_end_address;
}

pointer ArchCommon::getFreeKernelMemoryStart()
{
   return (pointer)&kernel_end_address;
}

pointer ArchCommon::getFreeKernelMemoryEnd()
{
   return (pointer)getModuleEndAddress(0);
}


uint32 ArchCommon::haveVESAConsole(uint32 is_paging_set_up __attribute__((unused)))
{
  return true;
}

uint32 ArchCommon::getNumModules(uint32 is_paging_set_up __attribute__((unused)))
{
  return 1;
}

uint32 ArchCommon::getModuleStartAddress(uint32 num __attribute__((unused)), uint32 is_paging_set_up __attribute__((unused)))
{
  return 0x80000000U;
}

uint32 ArchCommon::getModuleEndAddress(uint32 num __attribute__((unused)), uint32 is_paging_set_up __attribute__((unused)))
{
  return getKernelEndAddress();
}

const char* ArchCommon::getModuleName([[maybe_unused]]size_t num, [[maybe_unused]]size_t is_paging_set_up)
{
    return "kernel";
}

uint32 ArchCommon::getVESAConsoleHeight()
{
  return 480;
}

uint32 ArchCommon::getVESAConsoleWidth()
{
  return 640;
}

pointer ArchCommon::getVESAConsoleLFBPtr(uint32 is_paging_set_up __attribute__((unused)))
{
  return ArchBoardSpecific::getVESAConsoleLFBPtr();
}

pointer ArchCommon::getFBPtr(uint32 is_paging_set_up __attribute__((unused)))
{
  return getVESAConsoleLFBPtr();
}

uint32 ArchCommon::getVESAConsoleBitsPerPixel()
{
  return 16;
}

uint32 ArchCommon::getNumUseableMemoryRegions()
{
  return 1;
}

uint32 ArchCommon::getUseableMemoryRegion(uint32 region, pointer &start_address, pointer &end_address, uint32 &type)
{
  return ArchBoardSpecific::getUsableMemoryRegion(region, start_address, end_address, type);
}

Console* ArchCommon::createConsole(uint32 count)
{
  ArchBoardSpecific::frameBufferInit();
  return new FrameBufferConsole(count);
}

Stabs2DebugInfo const *kernel_debug_info = 0;

void ArchCommon::initDebug()
{
  extern unsigned char stab_start_address_nr;
  extern unsigned char stab_end_address_nr;

  extern unsigned char stabstr_start_address_nr;

  kernel_debug_info = new Stabs2DebugInfo((char const *)&stab_start_address_nr,
                                          (char const *)&stab_end_address_nr,
                                          (char const *)&stabstr_start_address_nr);

}

void ArchCommon::halt()
{
  asm("mcr p15, 0, %[v], c7, c0, 4" : : [v]"r" (0)); // Wait for interrupt
}

void ArchCommon::idle()
{
  ArchBoardSpecific::onIdle();
  halt();
}

void ArchCommon::spinlockPause()
{
}

uint64 ArchCommon::cpuTimestamp()
{
    uint64 timestamp;

    // // Read PMCCNTR/CCNT Register
    // // https://developer.arm.com/documentation/ddi0406/c/System-Level-Architecture/System-Control-Registers-in-a-PMSA-implementation/PMSA-System-control-registers-descriptions--in-register-order/PMCCNTR--Performance-Monitors-Cycle-Count-Register--PMSA?lang=en
    // asm volatile ("mrc p15, 0, %0, c9, c13, 0\n"
    //               : "=r"(timestamp));

    // // Read physical count register
    // // https://developer.arm.com/documentation/ddi0406/c/System-Level-Architecture/System-Control-Registers-in-a-PMSA-implementation/PMSA-System-control-registers-descriptions--in-register-order/CNTPCT--Physical-Count-register--PMSA?lang=en
    // asm("mrrc p15, 0, %Q0, %R0, c14\n"
    //     : "=r" (timestamp));

    // Read virtual count register
    // https://developer.arm.com/documentation/ddi0406/c/System-Level-Architecture/System-Control-Registers-in-a-PMSA-implementation/PMSA-System-control-registers-descriptions--in-register-order/CNTVCT--Virtual-Count-register--PMSA?lang=en
    asm("mrrc p15, 1, %Q0, %R0, c14\n"
        : "=r" (timestamp));

    return timestamp;
}

void ArchCommon::postBootInit()
{
}

extern "C" void __aeabi_atexit()
{
  assert(false && "would not make sense in a kernel");
}

extern "C" void __aeabi_unwind_cpp_pr0()
{
  assert(false && "no exception handling implemented");
}

extern "C" void raise()
{
  assert(false && "no exception handling implemented");
}

void ArchCommon::reservePagesPreKernelInit([[maybe_unused]]Allocator& alloc)
{
}
