#ifndef MINIX_STORAGE_MANAGER__
#define MINIX_STORAGE_MANAGER__

#include "StorageManager.h"
#include "types.h"

class MinixStorageManager : public StorageManager
{
  public:
    MinixStorageManager(char *bm_buffer, uint16 num_inode_bm_blocks, uint16 num_zone_bm_blocks, uint16 num_inodes, uint16 num_zones);
    virtual ~MinixStorageManager();
    virtual void *allocateMemory(uint32 size);
    virtual void freeMemory(void * /*data*/);
    bool isInodeSet(uint32 which) { return false; }
    uint32 getNumUsedInodes() { return 0; }
    
};


#endif //MINIX_STORAGE_MANAGER__

