#ifndef PAGEMANAGER_H__
#define PAGEMANAGER_H__

#include "types.h"
#include "paging-definitions.h"
#include "Mutex.h"
#include "Bitmap.h"

class PageManager
{
  public:
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
     * returns always 4kb ppns!
     */
    uint32 allocPPN(uint32 page_size = PAGE_SIZE);

    /**
     * marks physical page <page_number> as free, if it was used in
     * user or kernel space.
     * @param page_number Physcial Page to mark as unused
     */
    void freePPN(uint32 page_number, uint32 page_size = PAGE_SIZE);

    Thread* heldBy()
    {
      return lock_.heldBy();
    }

    PageManager();

    void printBitmap()
    {
      page_usage_table_->bmprint();
    }

  private:
    /**
     * used internally to mark pages as reserved
     * @param ppn
     * @param num
     * @return
     */
    bool reservePages(uint32 ppn, uint32 num = 1);

    PageManager(PageManager const&);

    Bitmap* page_usage_table_;
    uint32 number_of_pages_;
    uint32 lowest_unreserved_page_;

    Mutex lock_;

    static PageManager* instance_;

};

#endif
