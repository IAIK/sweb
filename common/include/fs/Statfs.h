/**
 * Filename: Statfs.h
 * Description:
 *
 * Created on: 23.08.2012
 * Author: chris
 */

#ifndef STATFS_H_
#define STATFS_H_

/**
 * @class statfs struct holds statistical informations about a mounted fs
 */
struct statfs_s
{
  uint8 fs_ident;                   // the partition identified of the FileSystem
  sector_len_t block_size;          // used block-size
  sector_addr_t num_blocks;         // number of sectors
  sector_addr_t num_free_blocks;    // number of free sectors

  inode_id_t max_inodes;            // the maximum number of i-nodes
  inode_id_t used_inodes;           // current number of used i-nodes

  // additional performance info
  Cache::CacheStat dev_cache_stat;  // device's cache stat
  Cache::CacheStat inode_cache_stat;// inode cache stat

  /**
   * compares two statfs_s object and return true if they are equal
   * they are defined as equal if their number of free-blocks and
   * used-inodes match
   */
  bool operator==(const statfs_s& cmp)
  {
    if(num_blocks == cmp.num_blocks && max_inodes == cmp.max_inodes &&
        num_free_blocks == cmp.num_free_blocks && used_inodes == cmp.used_inodes)
    {
      return true;
    }
    return false;
  }
};

#endif /* STATFS_H_ */
