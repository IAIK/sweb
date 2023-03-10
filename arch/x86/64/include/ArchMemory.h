#pragma once

#include "types.h"
#include <cstdint>
#include <cstddef>
#include "offsets.h"
#include "paging-definitions.h"
#include "EASTL/atomic.h"

extern PageMapLevel4Entry kernel_page_map_level_4[PAGE_MAP_LEVEL_4_ENTRIES] __attribute__((aligned(0x1000)));

struct ArchMemoryMapping
{
    PageMapLevel4Entry* pml4;
    PageDirPointerTableEntry* pdpt;
    PageDirEntry* pd;
    PageTableEntry* pt;
    pointer page;

    ppn_t pml4_ppn;
    ppn_t pdpt_ppn;
    ppn_t pd_ppn;
    ppn_t pt_ppn;
    ppn_t page_ppn;

    size_t page_size;

    uint64_t pml4i;
    uint64_t pdpti;
    uint64_t pdi;
    uint64_t pti;
};

class ArchMemory
{
public:
    ArchMemory();
    ArchMemory(ppn_t pml4_ppn);
    ~ArchMemory();

    /// Prevent accidental copying/assignment, can be implemented if needed
    ArchMemory(ArchMemory const &src) = delete;
    ArchMemory &operator=(ArchMemory const &src) = delete;

    /**
     * Maps a virtual page to a physical page and creates the upper paging-hierarchy tables on demand.
     * @param virtual_page The virtual page number (VPN) to map (not the full address)
     * @param physical_page The physical page number (PPN) which the virtual page should be mapped to
     * @param user_access page table entry flag indicating whether the virtual page should be accessible in user-mode
     * @return true if successful, false otherwise (the PT entry already exists)
     */
    [[nodiscard]] bool mapPage(vpn_t virtual_page, ppn_t physical_page, bool user_access);

    /**
     * Removes the mapping to a virtual_page by marking its page table entry as non-present and frees the mapped physical page.
     * Unmaps and de-allocates the upper paging-hierarchy tables if they are no longer required.
     * @param virtual_page The virtual page number (VPN) to be unmapped (not the full address)
     * @return Currently always returns true
     */
    bool unmapPage(vpn_t virtual_page);

    [[nodiscard]] static bool mapKernelPage(vpn_t virtual_page, ppn_t physical_page, bool can_alloc_pages = false, bool memory_mapped_io = false);
    static void unmapKernelPage(vpn_t virtual_page, bool free_page = true);

    /**
     * Takes a physical page number (PPN) and returns a virtual address that can be used to access the given physical page.
     * At kernel startup, all physical memory is mapped into a contiguous block of virtual memory in kernel space at a fixed offset.
     * This allows access to a physical addresses via the virtual address (IDENT_MAPPING_START + <phys addr>).
     * This technique is called identity mapping, 1:1 mapping or direct mapping (Linux)
     * https://wiki.osdev.org/Identity_Paging
     * @param ppn The physical page number
     * @param page_size Optional, defaults to 4k pages, but you need to set it to
     * 512 * 4096 or 512 * 512 * 4096 if you have a 2M or 1G page number. Not used in baseline SWEB.
     * @return Virtual Address above KERNEL_START pointing to the start of a memory segment that
     * acts as a 1:1 mapping to the given physical page
     */
    static pointer getIdentAddressOfPPN(ppn_t ppn, size_t page_size = PAGE_SIZE);
    static pointer getIdentAddress(size_t address); // TODO: rename to distinguish it from getIdentAddressOfPPN

    /**
     * Checks whether the given virtual address is valid and mapped to physical memory.
     * @param vaddress_to_check The virtual address to check (full address, not just the VPN)
     * @return physical address corresponding to the the virtual address if it is mapped, zero otherwise
     */
    pointer checkAddressValid(size_t vaddress_to_check) const;
    /**
     * Checks whether the given virtual address is valid and mapped to physical memory.
     * @param pml4 pml4 ppn to use as page table root for the check
     * @param vaddress_to_check The virtual address to check (full address, not VPN)
     * @return physical address (full address, not PPN) corresponding to the the virtual address if it is mapped, zero otherwise
     */
    static pointer checkAddressValid(ppn_t pml4, size_t vaddress_to_check);

    /**
     * Look at the page tables for the given virtual page (VPN) and collect information about the memory mapping.
     * Returns information about the final page table entry as well as the intermediate paging levels.
     * @param pml4 The pml4 of the ArchMemory object for which to translate the vpage
     * @param vpage The virtual page (VPN) to resolve
     * @return an ArchMemoryMapping object containing information about the memory mapping of the given virtual page
     */
    const ArchMemoryMapping resolveMapping(vpn_t vpage) const;
    /**
     * Retrieve information about the page table mapping for a given virtual address (see resolveMapping above).
     * This variant allows you to specify the pml4 to use as paging root.
     * @param pml4 the pml4 PPN to use as page table root
     * @param vpage The virtual page (VPN) to resolve
     * @return an ArchMemoryMapping object containing information about the memory mapping of the given virtual page
     */
    static const ArchMemoryMapping resolveMapping(ppn_t pml4, vpn_t vpage);

    static size_t get_PPN_Of_VPN_In_KernelMapping(vpn_t virtual_page, ppn_t* physical_page, ppn_t* physical_pte_page=0);

    /**
     * Get the value that should be stored in the %cr3 register in order to use this ArchMemory virtual address space
     *
     * @return value to be stored in cr3
     */
    size_t getValueForCR3() const;

    size_t getPagingStructureRootPhys() const;

    static PageMapLevel4Entry* getKernelPagingStructureRootVirt();
    static size_t getKernelPagingStructureRootPhys();

    /**
     * Load the %cr3 register with the given value to switch the virtual address space used by the CPU
     *
     * @param cr3_value new %cr3 value to be used
     */
    static void loadPagingStructureRoot(size_t cr3_value);

    /**
     * Flush TLB entries for the given virtual address (full address, not VPN)
     * on the current CPU
     *
     * @param addr virtual address to flush from TLB
     */
    static void flushLocalTranslationCaches(size_t addr);
    /**
     * Flush TLB entries for the given virtual address (full address, not VPN)
     * on all CPUs by sending TLB shootdown requests
     *
     * @param addr virtual address to flush from TLB
     */
    static void flushAllTranslationCaches(size_t addr);

    /**
     * Get the ArchMemory object used by kernel threads
     * (kernel space is the same for all threads)
     *
     * @return reference to the kernel ArchMemory object
     */
    static ArchMemory& kernelArchMemory();

private:

    /**
     * Check if the given page table is empty (no entries in use)
     *
     * @tparam T Page table type (level)
     * @tparam NUM_ENTRIES Number of entries in the page table
     * @param table virtual address of the page table
     * @return true if the table is empty
     * @return false if the table is still in use
     */
    template<typename T, size_t NUM_ENTRIES>
    static bool tableEmpty(T* table);

    /**
     * Clear an entry in a page table by setting it to non-present and clearing all other page table entry bits.
     *
     * @tparam T Page table type (level)
     * @param table virtual address of the page table
     * @param index index of the page table entry to clear
     */
    template<typename T>
    static void removeEntry(T* table, size_t index);

    /**
     * Fill a page table entry. Sets the corresponding page table entry fields and marks the entry as present.
     *
     * @tparam T Page table type (level)
     * @param table virtual address of the page table
     * @param index index of the page table entry to fill
     * @param ppn physical page number (PPN) the entry should map to
     * @param user_access whether the virtual page corresponding to the entry should be accessible in user mode
     * @param writeable whether the virtual page corresponding to the entry should be writeable
     * @param memory_mapped_io disable the cache and mark it as write-through for this page to prevent it from interfering with memory mapped hardware I/O
     */
    template <typename T>
    static void insert(T* table, size_t index, ppn_t ppn, bool user_access, bool writeable, bool memory_mapped_io);


    /**
     * The PPN of the pml4 root page table of this virtual address space
     */
    ppn_t page_map_level_4_;
};
