/**
 * Filename: MinixDefs.h
 * Description:
 *
 * Created on: 17.05.2012
 * Author: chris
 */

#ifndef MINIXDEFS_H_
#define MINIXDEFS_H_

// minix' default block size
#define MINIX_BLOCK_SIZE            1024
#define MINIX_SUPERBLOCK_OFFSET     1024

#define MINIX_INODE_SIZE            32
#define MINIX_V2_INODE_SIZE         64
#define MINIX_DIR_ENTRY_SIZE        32
#define MINIX_LINK_MAX              256   // NOTE: MINIX V1 can only store 256 links!

//#define MINIX_NAME_MAX              MINIX_MAX_FILENAME_LEN
//#define MINIX_PATH_MAX             TBD

#define MINIX_ROOT_INO              1

/// @qoute taken from the LinuxKernel source <linux/magic.h>
#define MINIX_SUPER_MAGIC   0x137F    /* minix v1 fs, 14 char names */
#define MINIX_SUPER_MAGIC2  0x138F    /* minix v1 fs, 30 char names */
#define MINIX2_SUPER_MAGIC  0x2468    /* minix v2 fs, 14 char names */
#define MINIX2_SUPER_MAGIC2 0x2478    /* minix v2 fs, 30 char names */
#define MINIX3_SUPER_MAGIC  0x4d5a    /* minix v3 fs, 60 char names */

#define MINIX_VALID_FS    0x0001    /* Clean fs. */
#define MINIX_ERROR_FS    0x0002    /* fs has errors. */

#define MINIX_MOUNT_FLAG_PENDING    0x00  // fs is currently mounted
#define MINIX_MOUNT_FLAG_OK         0x01
#define MINIX_MOUNT_FLAG_ERROR      0x02

#endif /* MINIXDEFS_H_ */
