#ifndef STORAGE_MANAGER__
#define STORAGE_MANAGER__

#include "../../../common/include/mm/Bitmap.h"
#include "types.h"

class StorageManager
{
  public:
    virtual ~StorageManager();
    virtual void *allocateMemory(uint32 size);
    virtual void freeMemory(void * /*data*/);
  
  private:
    Bitmap bitmap_;
};
#endif //STORAGE_MANAGER__
