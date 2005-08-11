//----------------------------------------------------------------------
//  $Id: xen_startup.c,v 1.2 2005/08/11 16:55:47 nightcreature Exp $
//----------------------------------------------------------------------
//
//  $Log: xen_startup.c,v $
//  Revision 1.1  2005/08/01 08:30:25  nightcreature
//  second boot stage based on kernel.c from mini-os
//
//
//----------------------------------------------------------------------


/******************************************************************************
 * kernel.c
 * 
 * Assorted crap goes here, including the initial C entry point, jumped at
 * from head.S.
 * 
 * Copyright (c) 2002-2003, K A Fraser & R Neugebauer
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */

#include "os.h"
#include "hypervisor.h"
#include "xen_memory.h"
#include "events.h"
#include "time.h"
#include "types.h"
#include "lib.h"

//#include "ArchCommon.h"


 extern void startup();
/*
 * Shared page for communicating with the hypervisor.
 * Events flags go here, for example.
 */
shared_info_t *HYPERVISOR_shared_info;

console_ready;

/*
 * This structure contains start-of-day info, such as pagetable base pointer,
 * address of the shared_info structure, and things like that.
 */
union start_info_union start_info_union;

/*
 * Just allocate the kernel stack here. SS:ESP is set up to point here
 * in head.S.
 */
char stack[8192];


/* Assembler interface fns in entry.S. */
void hypervisor_callback(void);
void failsafe_callback(void);

/* default event handlers */
static void exit_handler(int ev, struct pt_regs *regs);
static void debug_handler(int ev, struct pt_regs *regs);


extern char shared_info[PAGE_SIZE]; //defined in head.S

extern char text_start_address;

static shared_info_t *mapSharedInfo(unsigned long pa)
{
    if ( HYPERVISOR_update_va_mapping((unsigned long)shared_info >> PAGE_SHIFT,
                                      pa | 3, UVMF_INVLPG) )
    {
/*         printk("Failed to map shared_info!!\n"); */
        *(int*)0=0;
    }
    return (shared_info_t *)shared_info;
}


extern void print_message ()
{
    char buffer[256];
    int i;

    for (i = 0; i < 10; i++)
    {
        sprintf (buffer, "[%d] Hello World\n", i);
        kcons_write (buffer, strlen(buffer));
    }
}



/*
 * INITIAL C ENTRY POINT.
 */
void start_kernel(start_info_t *si)
{
    console_ready = 0;

    /* Copy the start_info struct to a globally-accessible area. */
    // start_info is defined in hypervisor.h
    memcpy(&start_info, si, sizeof(*si));

    // Grab the shared_info pointer and map it to our va space.
    //HYPERVISOR_shared_info defined as pointer in xen.h
    HYPERVISOR_shared_info = mapSharedInfo(start_info.shared_info);

    /* Set up event and failsafe callback addresses. */
    HYPERVISOR_set_callbacks(
        __KERNEL_CS, (unsigned long)hypervisor_callback,
        __KERNEL_CS, (unsigned long)failsafe_callback);

    //initalise memory maps in ArchCommon

    printf("initalising arch common memory maps\n");

    //----------------------------------------------------------------------
    //taken from mini-os mm.c

    uint64 start_pfn, max_pfn, max_free_pfn;
    
    uint64 *pgd = (unsigned long *)start_info.pt_base;
    max_pfn = start_info.nr_pages;
    start_pfn = PFN_UP(to_phys(& text_start_address));
    /*
     * we know where free tables start (start_pfn) and how many we 
     * have (max_pfn). 
     * 
     * Currently the hypervisor stores page tables it providesin the
     * high region of the this memory range.
     * 
     * next we work out how far down this goes (max_free_pfn)
     * 
     * XXX this assumes the hypervisor provided page tables to be in
     * the upper region of our initial memory. I don't know if this 
     * is always true.
     */

    max_free_pfn = PFN_DOWN(to_phys(pgd));
    {
        unsigned long *pgd = (unsigned long *)start_info.pt_base;
        unsigned long  pte;
        int i;

        for ( i = 0; i < (HYPERVISOR_VIRT_START>>22); i++ )
        {
            unsigned long pgde = *pgd++;
            if ( !(pgde & 1) ) continue;
            pte = machine_to_phys(pgde & PAGE_MASK);
            if (PFN_DOWN(pte) <= max_free_pfn) 
                max_free_pfn = PFN_DOWN(pte);
        }
    }
    max_free_pfn--;
    //end takeout from mini-os mm.c
    //----------------------------------------------------------------------
    
    initialiseArchCommonMemoryMaps(max_free_pfn - start_pfn);
    printf("arch common memory maps intalised\n");    
    
    /* init console driver */
    ctrl_if_init();
 
    trap_init();

    /* ENABLE EVENT DELIVERY. This is disabled at start of day. */
    __sti();
    
    /* print out some useful information  */
//      printk("Booting in Xen!\n"); 
//      printk("start_info:   %p\n",    si); 
//      printk("  nr_pages:   %lu",     si->nr_pages); 
//      printk("  shared_inf: %08lx\n", si->shared_info); 
//      printk("  pt_base:    %p",      (void *)si->pt_base);  
//      printk("  mod_start:  0x%lx\n", si->mod_start); 
//      printk("  mod_len:    %lu\n",   si->mod_len);  
//      printk("  flags:      0x%x\n",  (unsigned int)si->flags); 
//      printk("  cmd_line:   %s\n",   
//      si->cmd_line ? (const char *)si->cmd_line : "NULL"); 

    /*
     * If used for porting another OS, start here to figure out your
     * guest os entry point. Otherwise continue below...
     */

 
    //printmessage();
    printf("xen initialisation done, now entering sweb startup\n");    

    /* do nothing */
    //for ( ; ; ) HYPERVISOR_yield();

   /* entering sweb */
   startup();

}


/*
 * do_exit: This is called whenever an IRET fails in entry.S.
 * This will generally be because an application has got itself into
 * a really bad state (probably a bad CS or SS). It must be killed.
 * Of course, minimal OS doesn't have applications :-)
 */

void do_exit(void)
{
    printk("do_exit called!\n"); 
    for ( ;; ) HYPERVISOR_shutdown();
}

static void exit_handler(int ev, struct pt_regs *regs) {
    do_exit();
}

/*
 * a debug handler to print out some state from the guest
 */
static void debug_handler(int ev, struct pt_regs *regs) {
    dump_regs(regs);
}   
