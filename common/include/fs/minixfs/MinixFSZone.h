// Projectname: SWEB
// Simple operating system for educational purposes

#ifndef MINIX_FS_ZONE__
#define MINIX_FS_ZONE__

#include "types.h"

typedef uint16 zone_add_type;

class MinixFSSuperblock;

/**
 * @class MinixFSZone handles the zones of a minix inode
 * the minix file system's memory  is devided in zones, 
 * a zone is a combination of 1 to many blocks depending on the size of the file system
 * minix inodes have stored 9 zone adresses in its header
 * the first 7 are directly adressed to its first 7 zones
 * the 8th is the adress of a zone containing the zone adresses 8 to 520
 * the 9th is the adress of a zone containing the zone adresses for further zones containg the adresses 520 to 262,664
 */
class MinixFSZone
{
  public:
    
    /**
     * constructor
     * @param superblock the superblock
     * @param zones the zone array from the file system
     */
    MinixFSZone(MinixFSSuperblock *superblock, zone_add_type *zones);
    
    /**
     * destructor
     */
    ~MinixFSZone();
    
    /**
     * returns the zone adress at the given index
     * @param index the zone index
     * @return the adress
     */
    zone_add_type getZone(uint32 index);
    
    /**
     * sets the zone adress at the given index
     * @param index the index
     * @param zone the zone adress
     */
    void setZone(uint32 index, zone_add_type zone);
    
    /**
     * adds one zone with the given adress
     * @param zone the adress
     */
    void addZone(zone_add_type zone);
    
    /**
     * returns the number of zones
     * @return the number of zones
     */
    uint32 getNumZones() { return num_zones_; }

    /**
     * flushes the zones to the file system
     * @param i_num the inode number of the inode to flush the zones to
     */
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
