#pragma once

#include "types.h"
#include "Mutex.h"
#include "ArchMemory.h"
#include "ElfFormat.h"
#include "EASTL/vector.h"

class Stabs2DebugInfo;

/**
 * Responsible for managing the executable file for a process and loading data on demand
 */
class Loader
{
  public:
    /**
     * Create a new ELF executable loader with the given file descriptor
     *
     * @param fd an open file descriptor corresponding to the ELF executable
     */
    Loader(ssize_t fd);
    ~Loader();

    /**
     * Loads the ELF headers from the executable and loads debug info if available.
     * @return true if this was successful, false otherwise
     */
    bool loadExecutableAndInitProcess();

    /**
     * Load the data for the given virtual page from the ELF executable binary.
     * Allocates a new physical page, fills it with data from the executable and maps it into the virtual address space.
     * @param virtual_address virtual address that should be loaded
     */
    void loadPage(pointer virtual_address);

    Stabs2DebugInfo const* getDebugInfos() const;

    /**
     * Get the address of the entry function of the ELF executable
     */
    void* getEntryFunction() const;


    ArchMemory arch_memory_; // Object managing the virtual address space and page mappings of the process

  private:

    /**
     * Read ELF-headers from the executable
     * @return true on success, false otherwise
     */
    bool readHeaders();

    /**
     * Clean up and sort the elf headers for faster access.
     * @return true in case the headers could be prepared
     */
    bool prepareHeaders();

    bool loadDebugInfoIfAvailable();

    bool readFromBinary(char* buffer, l_off_t position, size_t length);

    size_t fd_; // file descriptor corresponding to the opened executable
    Elf::Ehdr *hdr_; // ELF header read from the executable file
    eastl::vector<Elf::Phdr> phdrs_; // ELF program headers describing the virtual address space layout of the program and file offsets where data should be loaded from
    Mutex program_binary_lock_;

    Stabs2DebugInfo *userspace_debug_info_;
};
