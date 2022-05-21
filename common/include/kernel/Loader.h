#pragma once

#include "types.h"
#include "Thread.h"
#include "Scheduler.h"
#include "Mutex.h"
#include "ArchMemory.h"
#include "ElfFormat.h"
#include <uvector.h>

class Stabs2DebugInfo;

class Loader
{
  public:
    Loader(ssize_t fd);
    ~Loader();

    /**
     *Initialises the Addressspace of the User, creates the Thread's
     *InfosUserspaceThread and sets the PageDirectory,
     *loads the ehdr and phdrs from executable
     * @return true if this was successful, false otherwise
     */
    bool loadExecutableAndInitProcess();

    /**
     * loads one page by its virtual address: gets a free page, copies the page, maps it
     * @param virtual_address virtual address where to find the page to load
     */
    void loadPage(pointer virtual_address);

    Stabs2DebugInfo const* getDebugInfos() const;

    void* getEntryFunction() const;

    ArchMemory arch_memory_;

  private:

    /**
     *reads ELF-headers from the executable
     * @return true if this was successful, false otherwise
     */
    bool readHeaders();


    /**
     * clean up and sort the elf headers for faster access.
     * @return true in case the headers could be prepared
     */
    bool prepareHeaders();


    bool loadDebugInfoIfAvailable();


    bool readFromBinary (char* buffer, l_off_t position, size_t length);


    size_t fd_;
    Elf::Ehdr *hdr_;
    ustl::vector<Elf::Phdr> phdrs_;
    Mutex program_binary_lock_;

    Stabs2DebugInfo *userspace_debug_info_;

};
