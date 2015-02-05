#ifndef MINIX_FS_ZONE__
#define MINIX_FS_ZONE__

#include "types.h"

class MinixFSSuperblock;

/**
 * @class MinixFSZone handles the zones of a minix inode
 * the minix file system's memory  is devided in zones,
 * a zone is a combination of 1 to many blocks depending on the size of the file system
 * minix inodes have stored 9 zone addresses in its header
 * the first 7 are directly addressed to its first 7 zones
 * the 8th is the address of a zone containing the zone addresses 8 to 520
 * the 9th is the address of a zone containing the zone addresses for further zones containg the addresses 520 to 262,664
 */
class MinixFSZone
{
  public:

    /**
     * constructor
     * @param superblock the superblock
     * @param zones the zone array from the file system
     */
    MinixFSZone(MinixFSSuperblock *superblock, uint32 *zones);

    /**
     * destructor
     */
    ~MinixFSZone();

    /**
     * returns the zone address at the given index
     * @param index the zone index
     * @return the address
     */
    uint32 getZone(uint32 index);

    /**
     * sets the zone address at the given index
     * @param index the index
     * @param zone the zone address
     */
    void setZone(uint32 index, uint32 zone);

    /**
     * adds one zone with the given address
     * @param zone the address
     */
    void addZone(uint32 zone);

    /**
     * returns the number of zones
     * @return the number of zones
     */
    uint32 getNumZones()
    {
      return num_zones_;
    }

    /**
     * flushes the zones to the file system
     * @param i_num the inode number of the inode to flush the zones to
     */
    void flush(uint32 i_num);

    /**
     * frees all zones, also the ones which are storing zone addresses
     */
    void freeZones();

  private:

    MinixFSSuperblock *superblock_;
    uint32 direct_zones_[10];
    uint32 *indirect_zones_;
    uint32 *double_indirect_linking_zone_;
    uint32 **double_indirect_zones_;

    uint32 num_zones_;

};

#endif //MINIX_FS_ZONE__
