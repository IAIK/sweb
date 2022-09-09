#include "ArchMemory.h"
#include "ArchMulticore.h"
#include "ArchInterrupts.h"
#include "ArchCommon.h"
#include "SMP.h"
#include "offsets.h"
#include "debug.h"

// Common functionality for x86 memory management (x86_64 / x86_32 / x86_32_pae)

pointer ArchMemory::getIdentAddressOfPPN(ppn_t ppn, size_t page_size /* optional */)
{
    return IDENT_MAPPING_START + (ppn * page_size);
}

pointer ArchMemory::getIdentAddress(size_t address)
{
    return IDENT_MAPPING_START | (address);
}

size_t ArchMemory::getKernelPagingStructureRootPhys()
{
    return (size_t)VIRTUAL_TO_PHYSICAL_BOOT(getKernelPagingStructureRootVirt());
}

size_t ArchMemory::getValueForCR3() const
{
    return getPagingStructureRootPhys();
}

void ArchMemory::loadPagingStructureRoot(size_t cr3_value)
{
    __asm__ __volatile__("mov %[cr3_value], %%cr3\n"
                         ::[cr3_value]"r"(cr3_value));
}

void ArchMemory::flushLocalTranslationCaches(size_t addr)
{
    if(A_MEMORY & OUTPUT_ADVANCED)
    {
        debug(A_MEMORY, "CPU %zx flushing translation caches for address %zx\n", SMP::currentCpuId(), addr);
    }
    __asm__ __volatile__("invlpg %[addr]\n"
                         ::[addr]"m"(*(char*)addr));
}

eastl::atomic<size_t> shootdown_request_counter;

void ArchMemory::flushAllTranslationCaches(size_t addr)
{

        assert(SMP::cpu_list_.size() >= 1);
        eastl::vector<TLBShootdownRequest> shootdown_requests{SMP::cpu_list_.size()};
        //TLBShootdownRequest shootdown_requests[ArchMulticore::cpu_list_.size()]; // Assuming the kernel stack is large enough as long as we only have a few CPUs

        bool interrupts_enabled = ArchInterrupts::disableInterrupts();
        flushLocalTranslationCaches(addr);

        auto orig_cpu = SMP::currentCpuId();

        ((char*)ArchCommon::getFBPtr())[2*80*2 + orig_cpu*2] = 's';

        /////////////////////////////////////////////////////////////////////////////////
        // Thread may be re-scheduled on a different CPU while sending TLB shootdowns  //
        // This is fine, since a context switch also invalidates the TLB               //
        // A CPU sending a TLB shootdown to itself is also fine                        //
        /////////////////////////////////////////////////////////////////////////////////
        size_t request_id = ++shootdown_request_counter;

        for(auto& r : shootdown_requests)
        {
                r.addr = addr;
                r.ack = 0;
                r.target = (size_t)-1;
                r.next = nullptr;
                r.orig_cpu = orig_cpu;
                r.request_id = request_id;
        }


        shootdown_requests[orig_cpu].ack |= (1 << orig_cpu);

        size_t sent_shootdowns = 0;

        for(auto& cpu : SMP::cpu_list_)
        {
                size_t cpu_id = cpu->id();
                shootdown_requests[cpu_id].target = cpu_id;
                if(cpu->id() != orig_cpu)
                {
                        debug(A_MEMORY, "CPU %zx Sending TLB shootdown request %zx for addr %zx to CPU %zx\n", SMP::currentCpuId(), shootdown_requests[cpu_id].request_id, addr, cpu_id);
                        assert(SMP::currentCpuId() == orig_cpu);
                        assert(cpu_id != orig_cpu);
                        sent_shootdowns |= (1 << cpu_id);

                        TLBShootdownRequest* expected_next = nullptr;
                        do
                        {
                                shootdown_requests[cpu_id].next = expected_next;
                        } while(!cpu->tlb_shootdown_list.compare_exchange_weak(expected_next, &shootdown_requests[cpu_id]));

                        assert(cpu->lapic->ID() == cpu_id);
                        asm("mfence\n");
                        cpu_lapic.sendIPI(99, *cpu->lapic, true);
                }
        }
        assert(!(sent_shootdowns & (1 << orig_cpu)));

        debug(A_MEMORY, "CPU %zx sent %zx TLB shootdown requests, waiting for ACKs\n", SMP::currentCpuId(), sent_shootdowns);

        if(interrupts_enabled) ArchInterrupts::enableInterrupts();

        for(auto& r : shootdown_requests)
        {
                if(r.target != orig_cpu)
                {
                        do
                        {
                                assert((r.ack.load() &~ sent_shootdowns) == 0);
                        }
                        while(r.ack.load() == 0);
                }
        }

        ((char*)ArchCommon::getFBPtr())[2*80*2 + orig_cpu*2] = ' ';
}
