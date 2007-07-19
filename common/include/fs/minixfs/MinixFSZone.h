#ifndef MINIX_FS_ZONE__
#define MINIX_FS_ZONE__

#include "types.h"

typedef uint16 zone_add_type;

class MinixFSSuperblock;

class MinixFSZone
{
  public:
    MinixFSZone(MinixFSSuperblock *superblock, zone_add_type *zones);
    ~MinixFSZone();
    
    zone_add_type getZone(uint32 index);
    void setZone(uint32 index, zone_add_type zone);
    void addZone(zone_add_type zone);
    
    uint32 getNumZones() { return num_zones_; }

    void flush(uint32 i_num);
    
  private:
    MinixFSSuperblock *superblock_;
    zone_add_type *direct_zones_;
    zone_add_type *indirect_zones_;
    zone_add_type *double_indirect_linking_zone_;
    zone_add_type **double_indirect_zones_;
    
    uint32 num_zones_;
  
};

#endif //MINIX_FS_ZONE__
