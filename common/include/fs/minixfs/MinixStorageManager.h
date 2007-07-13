#ifndef MINIX_STORAGE_MANAGER__
#define MINIX_STORAGE_MANAGER__

//#include "../../../../arch/common/include/StorageManager.h"
#include "StorageManager.h"
#include "types.h"
#include "../../util/Buffer.h"

#define MINIX_BLOCK_SIZE 1024
#define MINIX_ZONE_SIZE 1024
#define INODES_PER_BLOCK 32

class MinixStorageManager : public StorageManager
{
  public:
    MinixStorageManager(Buffer *bm_buffer, uint16 num_inode_bm_blocks, uint16 num_zone_bm_blocks, uint16 num_inodes, uint16 num_zones);
    virtual ~MinixStorageManager();
    virtual size_t acquireZone();
    virtual void freeZone(size_t index);
    bool isInodeSet(size_t index);
    uint32 getNumUsedInodes();
    size_t curr_zone_pos_;
    //debug
    void printBitmap();
    
};


#endif //MINIX_STORAGE_MANAGER__

