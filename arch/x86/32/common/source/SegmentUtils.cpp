/**
 * @file SegmentUtils.cpp
 *
 */

#include "SegmentUtils.h"
#include "ArchCommon.h"
#include "kprintf.h"

class Thread;

extern uint32 core0_local_storage;
extern uint32 core1_local_storage;
extern uint32 tss_selector;
extern uint32 core_local_selector;
extern __thread Thread *currentThread;
//extern uint32 gdt_ptr_new;

extern "C" void reload_segements();
extern "C" void reload_segments_core1();

typedef struct {
    uint16 limitL;
    uint16 baseL;
    uint8 baseM;
    uint8 type;
    uint8 limitH;
    uint8 baseH;
} __attribute__((__packed__))SegDesc;


__thread TSS *g_tss;
TSS* tss_core0;
TSS* tss_core1;


#define GDT_ENTRY_NUM 8192
#define GDT_SIZE      (GDT_ENTRY_NUM * 8)

#define SEGMENT_ABSENT  0x00
#define SEGMENT_PRESENT 0x80
#define SEGMENT_DPL0    0x00
#define SEGMENT_DPL1    0x20
#define SEGMENT_DPL2    0x40
#define SEGMENT_DPL3    0x60

static void setTSSSegDesc(uint32 base, uint32 limit, uint8 type, int8 core)
{
    SegDesc *desc = ((SegDesc*)&tss_selector)+core;

    desc->baseL  = (uint16)(base & 0xFFFF);
    desc->baseM  = (uint8)((base >> 16U) & 0xFF);
    desc->baseH  = (uint8)((base >> 24U) & 0xFF);
    desc->limitL = (uint16)(limit & 0xFFFF);
    // 4KB unit & 32bit segment
    desc->limitH = (uint8) (((limit >> 16U) & 0x0F) | 0xC0); 
    desc->type   = type;
    return;
}

void SegmentUtils::load_gs(int8 core)
{
	void* core_local_storage_end =  (core == 0 ? &core0_local_storage : &core1_local_storage) - 4096;

	asm volatile("mov    %0,%%eax\n"
                 "mov %%eax,%%gs:0x0\n":: "m" (core_local_storage_end));
}

static void setCoreLocalStorageSel(int8 core)
{
    SegDesc *desc = ((SegDesc*)&core_local_selector)+core;

    uint32 core_local_storage = (uint32) (core ? &core1_local_storage : &core0_local_storage);
    // TODO: dynamic
    // uint32 core_local_storage = (uint32) (&core0_local_storage + core * 1024);

    debug ( MC_INIT, "Core %d core local storage sel; desc = %x, local storage = %x\n", core, desc, core_local_storage);

    desc->baseL  = (uint16)((uint32)core_local_storage & 0xFFFF);
    desc->baseM  = (uint8)(((uint32)core_local_storage >> 16U) & 0xFF);
    desc->baseH  = (uint8)(((uint32)core_local_storage >> 24U) & 0xFF);
    desc->limitL = (uint16)(0x1);
    desc->limitH = (uint8)(0x0CF);	//0CFh  ; 1 1 0 0 granularity:page (4 gig limit), 32-bit, not a 64 bit code segment;
    desc->type   = (uint8)(0x92);	//92h   ; 1 00 1 0010 == present, ring 0, data, expand-up, writable;
    return;
}

void SegmentUtils::initialise()
{
  SegmentUtils::initialiseCore(0, tss_core0);
  setCoreLocalStorageSel(0);

  SegmentUtils::initialiseCore(1, tss_core1);
  setCoreLocalStorageSel(1);

  debug ( MC_INIT, "Core 0 global tss is %x\n",tss_core0);
  debug ( MC_INIT, "Core 1 global tss is %x\n",tss_core1);

	/*
  for (int i = 0; i < 2; i++)
  {
	  SegmentUtils::initialiseCore(i);
	  setCoreLocalStorageSel(i);
  }
  */
}

void SegmentUtils::initialiseCore(uint32 core, TSS* &tss)
{
  tss = (TSS*)new uint8[sizeof(TSS)];
  ArchCommon::bzero((pointer)tss,sizeof(TSS));

  tss->ss0 = KERNEL_SS;
  setTSSSegDesc((uint32)tss, 0x00000067, SEGMENT_PRESENT | SEGMENT_DPL0 | 0x00 | 0x09, core);

  debug ( MC_INIT, "Core %d initialised tss %x\n", core, tss);
}

void SegmentUtils::load(int8 core)
{
  // we have to reload our segment stuff
  // TODO: (6 + COUNT_CORES)
  //uint16 val = (8 * (6 + 2)) + (core * 8);

	// TODO: or maybe just 1??
  uint16 val = (8 * (6 + 2)) + (core * 8);

  debug ( MC_INIT, "Core %d reloading segments\n", core);

  if (core == 0)
  {
	  reload_segements();
	  load_gs(0);
	  g_tss = tss_core0;
  }
  else
  {
	  reload_segments_core1();
	  load_gs(1);
	  g_tss = tss_core1;
  }
  debug ( MC_INIT, "Core %d uses tss %x\n", core, g_tss);

   // now use our damned tss
   asm volatile("ltr %0\n": "=m" (val));

   debug ( MC_INIT, "Core %d done loading tr\n", core);
}
