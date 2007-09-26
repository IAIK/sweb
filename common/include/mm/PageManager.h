/**
 * @file PageManger.h
 */

#ifndef PAGEMANAGER_H__
#define PAGEMANAGER_H__

#include "types.h"
#include "paging-definitions.h"
#include "new.h"
#include "kernel/Mutex.h"

typedef uint8 puttype;
#define PAGE_RESERVED static_cast<puttype>(1<<0)
#define PAGE_KERNEL static_cast<puttype>(1<<1)
#define PAGE_USERSPACE static_cast<puttype>(1<<2)


#define PAGE_FREE static_cast<puttype>(0)

/**
 * @class PageManager is in issence a BitMap managing free or used pages of size PAGE_SIZE only
 */
class PageManager
{
  public:

    /**
     * Creates a new instance (and THE ONLY ONE) of our page manager
     * This one will also automagically initialise itself accordingly
     * i.e. mark pages used by the kernel already as used
     * @param next_usable_address Pointer to memory the page manager can use
     * @return 0 on failure, otherwise a pointer to the next free memory location
     */
    static pointer createPageManager ( pointer next_usable_address );

    /**
    * the access method to the singleton instance
    * @return the instance
     */
    static PageManager *instance() {return instance_;}

    /**
     * returns the number of 4k Pages avaible to sweb.
     * ((size of the first usable memory region or max 1Gb) / PAGESIZE)
     * @return number of avilable pages
     */
    uint32 getTotalNumPages() const;

    /**
     * returns the number of the next free Physical Page
     * and marks that Page as used.
     * @param type can be either PAGE_USERSPACE (default) or PAGE_KERNEL
     * (passing PAGE_RESERVED or PAGE_FREE would obviously not make much sense,
     * since the page then either can neve be free'd again or will be given out
     * again)
     */
    uint32 getFreePhysicalPage ( uint32 type = PAGE_USERSPACE ); //also marks page as used

    /**
     * marks physical page <page_number> as free, if it was used in
     * user or kernel space.
     * @param page_number Physcial Page to mark as unused
     */
    void freePage ( uint32 page_number );

    /**
     * call this after initializing the KernelMemoryManager and before
     * starting Interrupts to ensure Mutual Exclusion
     */
    void startUsingSyncMechanism() {lock_=new Mutex();}

  private:

    /**
     * returns the size of used memory
     * @return the size
     */
    uint32 getSizeOfMemoryUsed() const;

    /**
     * the singleton constructor
     * @param start_of_structure the start address of the memory after the page manager
     */
    PageManager ( pointer start_of_structure );

    /**
     * Copy Constructor
     * must not be implemented
     */
    PageManager ( PageManager const& ) {};
    //PageManager &operator=(PageManager const&){};
    static PageManager* instance_;

    puttype  *page_usage_table_;
    uint32 number_of_pages_;

    Mutex *lock_;

};

#endif
