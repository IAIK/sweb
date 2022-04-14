#pragma once

// These correspond to the inode type enum values in the kernel (need to be kept in sync)
#define DT_REG         0
#define DT_DIR         1
#define DT_LINK        2
#define DT_CHR         3
#define DT_BLK         4

typedef struct
{
    unsigned long long d_offs_next; // offset to the next entry (from start of this entry)
    unsigned char d_type;           // file type
    char d_name[];                  // file name (null terminated)
} __attribute__((packed)) dirent;
