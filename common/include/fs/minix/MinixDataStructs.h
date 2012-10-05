/**
 * Filename: MinixDataStructs.h
 * Description:
 *
 * Created on: 17.05.2012
 * Author: chris
 */

#ifndef MINIXDATASTRUCTS_H_
#define MINIXDATASTRUCTS_H_

/**
 * NOTE: data-structures are taken from the Linux-Kernel
 * and integer-typedefs are adapted for SWEB
 * /include/linux/minix_fs.h
 */

/**
 * Minix-Superblock
 */
struct minix_super_block
{
    uint16 s_ninodes;
    uint16 s_nzones;
    uint16 s_imap_blocks;
    uint16 s_zmap_blocks;
    uint16 s_firstdatazone;
    uint16 s_log_zone_size;
    uint32 s_max_size;
    uint16 s_magic;
    uint16 s_state;
    uint32 s_zones;
};

/* File types.  */
#define S_IFDIR     0040000 /* Directory.  */
#define S_IFCHR     0020000 /* Character device.  */
#define S_IFBLK     0060000 /* Block device.  */
#define S_IFREG     0100000 /* Regular file.  */
#define S_IFIFO     0010000 /* FIFO.  */
#define S_IFLNK     0120000 /* Symbolic link.  */
#define S_IFSOCK    0140000 /* Socket.  */

/**
 * Minix-I-node
 *
 * Details and explanations
 *
 * i_mode - type of the I-Node
 *
 * i_zone - there are 9 data-block (aka zone) informations stored for each
 * i-node. The first 7 (indices ranging from 0-6) are 16bit addresses
 * to data sectors on the device. Address 8 points to an indirect block
 * (which contains BLOCK_SIZE/2 direct pointers). Entry 9 points to a double
 * indirect block. Where this block points to BLOCK_SIZE/2 indirect blocks.
 */
struct minix_inode
{
    uint16 i_mode;
    uint16 i_uid;
    uint32 i_size;
    uint32 i_time;
    uint8  i_gid;
    uint8  i_nlinks;
    uint16 i_zone[9];
};

/*
 * NOTE: taken from linux/minix_fs.h and adapted!
 *
 * The new minix inode has all the time entries, as well as
 * long block numbers and a third indirect block (7+1+1+1
 * instead of 7+1+1). Also, some previously 8-bit values are
 * now 16-bit. The inode is now 64 bytes instead of 32.
 */
struct minix2_inode {
  uint16 i_mode;
  uint16 i_nlinks;
  uint16 i_uid;
  uint16 i_gid;
  uint32 i_size;
  uint32 i_atime;
  uint32 i_mtime;
  uint32 i_ctime;
  uint32 i_zone[10];
};

// minix' max filename length
#define MINIX_MAX_FILENAME_LEN      30

struct minix_dirent
{
    uint16 inode_no;    // I-Node number
    char filename[MINIX_MAX_FILENAME_LEN];  // filename max 30-chars
};

#endif /* MINIXDATASTRUCTS_H_ */
