#ifndef STORAGE_MANAGER__
#define STORAGE_MANAGER__

#include "../../../common/include/mm/Bitmap.h"
#include "types.h"

class StorageManager
{
  public:
    StorageManager(uint16 num_inodes, uint16 num_zones);
//     StorageManager(){};
    virtual ~StorageManager();
    virtual void freeZone(size_t index) = 0;
    virtual bool isInodeSet(size_t index) = 0;
  
  protected:
    Bitmap* inode_bitmap_;
    Bitmap* zone_bitmap_;
};
#endif //STORAGE_MANAGER__
