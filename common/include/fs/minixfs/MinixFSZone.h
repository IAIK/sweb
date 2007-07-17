#ifndef MINIX_FS_ZONE__
#define MINIX_FS_ZONE__

#include "types.h"


#define NUM_ZONE_ADDRESSES 512
#define ZONE_SIZE 1024

class MinixFSSuperblock;

class MinixFSZone
{
  public:
    MinixFSZone(MinixFSSuperblock *superblock, uint16 *zones);
    ~MinixFSZone();
    
    uint16 getZone(uint32 index);
    void setZone(uint32 index, uint16 zone);
    void addZone(uint16 zone);
    
    uint32 getNumZones() { return num_zones_; }
    
  private:
    MinixFSSuperblock *superblock_;
    uint16 *direct_zones_;
    uint16 *indirect_zones_;
    uint16 *double_indirect_linking_zone_;
    uint16 **double_indirect_zones_;
    
    uint32 num_zones_;
  
};

#endif //MINIX_FS_ZONE__
