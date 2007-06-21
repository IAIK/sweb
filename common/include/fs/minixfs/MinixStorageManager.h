#ifndef MINIX_STORAGE_MANAGER__
#define MINIX_STORAGE_MANAGER__

#include "StorageManager.h"
#include "types.h"

class MinixStorageManager : public StorageManager
{
  public:
    virtual ~MinixStorageManager();
    virtual void *allocateMemory(uint32 size);
    virtual void freeMemory(void * /*data*/);
  
  private:
    Bitmap bitmap_;
};


#endif //MINIX_STORAGE_MANAGER__

