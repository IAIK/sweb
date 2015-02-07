#ifndef MINIX_FS_CONSTS__
#define MINIX_FS_CONSTS__

#define ZONE_BYTES(SB) (((SB)->s_magic_==MINIX_V3) ? 4 : 2)
#define ZONE_CAST(SB,BUF) (((SB)->s_magic_==MINIX_V3) ? (uint32*)(BUF) : (uint16*)(BUF))
#define NUM_ZONE_ADDRESSES(SB) (((SB)->s_magic_==MINIX_V3) ? 256 : 512)
#define ZONE_SIZE 1024
#define BLOCK_SIZE 1024
#define INODE_SIZE(SB) (((SB)->s_magic_==MINIX_V3) ? 64 : 32)
#define INODES_PER_BLOCK(SB) (((SB)->s_magic_==MINIX_V3) ? 16 : 32)
#define DENTRY_SIZE(SB) (((SB)->s_magic_==MINIX_V3) ? 64 : 32)
#define MAX_NAME_LENGTH(SB) (((SB)->s_magic_==MINIX_V3) ? 60 : 30)
#define NUM_ZONES(SB) (((SB)->s_magic_==MINIX_V3) ? 10 : 9)
#define MINIX_V3 0x4d5a

#endif
