#ifndef MINIX_FS_CONSTS__
#define MINIX_FS_CONSTS__

#define NUM_ZONE_ADDRESSES(X) ((X==MINIX_V3) ? 256 : 512)
#define ZONE_SIZE 1024
#define BLOCK_SIZE 1024
#define INODE_SIZE(X) ((X==MINIX_V3) ? 64 : 32)
#define INODES_PER_BLOCK(X) ((X==MINIX_V3) ? 16 : 32)
#define DENTRY_SIZE(X) ((X==MINIX_V3) ? 64 : 32)
#define MAX_NAME_LENGTH(X) ((X==MINIX_V3) ? 60 : 30)
#define MINIX_V3 0x4d5a

#endif
