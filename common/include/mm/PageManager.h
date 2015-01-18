/**
 * @file PageManger.h
 */

#ifndef PAGEMANAGER_H__
#define PAGEMANAGER_H__

#include "types.h"
#include "paging-definitions.h"
#include "Mutex.h"

class Bitmap;

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
    static void createPageManager ();

    /**
    * the access method to the singleton instance
    * @return the instance
     */
    static PageManager *instance();

    /**
     * returns the number of 4k Pages avaible to sweb.
     * ((size of the first usable memory region or max 1Gb) / PAGESIZE)
     * @return number of avilable pages
     */
    uint32 getTotalNumPages() const;

    /**
     * returns the number of the lowest free Page
     * and marks that Page as used.
     */
    uint32 getFreePhysicalPage(uint32 page_size = PAGE_SIZE); //marks page as used
    bool reservePages(uint32 ppn, uint32 num = 1); // used internally to mark as reserved

    /**
     * marks physical page <page_number> as free, if it was used in
     * user or kernel space.
     * @param page_number Physcial Page to mark as unused
     */
    void freePage(uint32 page_number, uint32 page_size = PAGE_SIZE);

  private:

    /**
     * the singleton constructor
     * @param start_of_structure the start address of the memory after the page manager
     */
    PageManager ();

    /**
     * Copy Constructor
     * must not be implemented
     */
    PageManager ( PageManager const& );

    static PageManager* instance_;

    Bitmap* page_usage_table_;
    uint32 number_of_pages_;
    uint32 lowest_unreserved_page_;

    Mutex lock_;

};

#endif
