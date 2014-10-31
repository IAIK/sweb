/**
 * @file Loader.h
 */

#ifndef __LOADER_H__
#define __LOADER_H__

#include "types.h"
#include "Thread.h"
#include "Scheduler.h"
#include "Mutex.h"
#include "ArchMemory.h"
#include "ElfFormat.h"
#include <ustl/uvector.h>

/**
* @class Loader manages the Addressspace creation of a thread
*/
class Loader
{
  public:

    /**
     *Constructor
     * @param fd the file descriptor of the executable
     * @param thread Thread to which the loader should belong
     * @return Loader instance
     */
    Loader(ssize_t fd, Thread *thread);

    /**
     *Destructor
     */
    ~Loader();

    /**
     *Creates a new PageDirectory and a new Page for the stack and initialises them
     */
    void initUserspaceAddressSpace();

    /**
     *Initialises the Addressspace of the User, creates the Thread's
     *InfosUserspaceThread and sets the PageDirectory,
     *loads the ehdr and phdrs from executable
     *
     * @return true if this was successful, false otherwise
     */
    bool loadExecutableAndInitProcess();

    /**
     *loads one page slow by its virtual address: gets a free page, maps it,
     *zeros it out, copies the page, one byte at a time
     * @param virtual_address virtual address where to find the page to load
     */
    void loadOnePageSafeButSlow ( pointer virtual_address );

    ArchMemory arch_memory_;

  private:

    /**
     *reads ELF-headers from the executable
     * @return true if this was successful, false otherwise
     */
    bool readHeaders();


    size_t fd_;
    Thread *thread_;
    Elf::Ehdr *hdr_;
    ustl::vector<Elf::Phdr> phdrs_;
    Mutex load_lock_;
};

#endif
