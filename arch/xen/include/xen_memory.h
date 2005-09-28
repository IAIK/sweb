//----------------------------------------------------------------------
//  $Id: xen_memory.h,v 1.3 2005/09/28 16:35:43 nightcreature Exp $
//----------------------------------------------------------------------
//
//  $Log: xen_memory.h,v $
//  Revision 1.2  2005/08/11 18:09:46  nightcreature
//  *** empty log message ***
//
//  Revision 1.1  2005/08/11 16:59:10  nightcreature
//  replacing mm.h
//
//
//----------------------------------------------------------------------


// partly based on mm.h by  Rolf Neugebauer (neugebar@dcs.gla.ac.uk)from mini-os

/*
 ****************************************************************************
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

#ifndef _XEN_MEMORY_H_
#define _XEN_MEMORY_H_

#include "paging-definitions.h"

#define PAGE_SHIFT      12
//#define PAGE_SIZE       (1UL << PAGE_SHIFT)
#define PAGE_MASK       (~(PAGE_SIZE-1))

#define PFN_UP(x)	(((x) + PAGE_SIZE-1) >> PAGE_SHIFT)
#define PFN_DOWN(x)	((x) >> PAGE_SHIFT)
#define PFN_PHYS(x)	((x) << PAGE_SHIFT)

#define VIRT_START              0x80000000UL

#define to_phys(x)                 ((unsigned long)(x)-VIRT_START)
#define to_virt(x)                 ((void *)((unsigned long)(x)+VIRT_START))

/* to align the pointer to the (next) page boundary */
#define PAGE_ALIGN(addr)        (((addr)+PAGE_SIZE-1)&PAGE_MASK)

//machine_to_phys_mapping is defined in xen.h
#define mfn_to_pfn(_mfn) (machine_to_phys_mapping[(_mfn)])

//initalised in initalisePhysToMachineMapping()
extern unsigned long *phys_to_machine_mapping;
#define pfn_to_mfn(_pfn) (phys_to_machine_mapping[(_pfn)])


void initalisePhysToMachineMapping();

void initalisePhysMapping3GB(uint32 nr_pages);

static __inline__ unsigned long phys_to_machine(unsigned long phys)
{
    unsigned long machine = pfn_to_mfn(phys >> PAGE_SHIFT);
    machine = (machine << PAGE_SHIFT) | (phys & ~PAGE_MASK);
    return machine;
}
static __inline__ unsigned long machine_to_phys(unsigned long machine)
{
    unsigned long phys = mfn_to_pfn(machine >> PAGE_SHIFT);
    phys = (phys << PAGE_SHIFT) | (machine & ~PAGE_MASK);
    return phys;
}

// void init_mm(void);
// unsigned long alloc_pages(int order);

#endif /* _XEN_HEMORY_ */
