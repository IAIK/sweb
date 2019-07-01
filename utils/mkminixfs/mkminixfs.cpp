#ifdef EXE2MINIXFS
#include "types.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>

#include "Dentry.h"
#include "FileSystemInfo.h"
#include "Superblock.h"
#include "MinixFSSuperblock.h"
#include "VfsSyscall.h"
#include "VfsMount.h"

Superblock* superblock_;
FileSystemInfo* default_working_dir;
VfsMount vfs_dummy_;

typedef struct fdisk_partition
{
  uint8 bootid; // bootable?  0=no, 128=yes
  uint8 beghead;
  uint8 begcyl;
  uint8 begsect;
  uint8 systid; // Operating System type indicator code
  uint8 endhead;
  uint8 endcyl;
  uint8 endsect;
  uint32 relsect; // first sector relative to start of disk
  uint32 numsect; // number of sectors in partition
} FP;

typedef struct master_boot_record
{
  uint8 bootinst[446]; // GRUB space
  uint8 parts[4 * sizeof(FP)];
  uint16 signature; // set to 0xAA55 for PC MBR
} MBR;

FileSystemInfo* getcwd() { return default_working_dir; }

// obviously NOT atomic, we need this for compatability in single threaded host code
size_t atomic_add(size_t& x,size_t y)
{
  x += y;
  return x-y;
}

int main(int argc, char *argv[])
{
  int c;
  char* device = NULL;
  unsigned long partition = 0;
  __attribute__((unused)) int have_partition = 0;
  while ((c = getopt(argc, argv, "d:p:")) != -1)
    switch (c)
    {
    case 'd':
      device = optarg;
      break;
    case 'p':
      partition = strtol(optarg, NULL, 10);
      have_partition = 1;
      break;
    case '?':
      if (optopt == 'd')
        fprintf (stderr, "mkminixfs: Option -%c requires an argument.\n", optopt);
      else if (isprint (optopt))
        fprintf (stderr, "mkminixfs: Unknown option `-%c'.\n", optopt);
      else
        fprintf (stderr,
                 "mkminixfs: Unknown option character `\\x%x'.\n",
                 optopt);
      return 1;
    default:
      exit(-1);
    }

  if(!device)
  {
    printf("mkminixfs: Usage: %s -d <device> [-p partition]\n", argv[0]);
    exit(-1);
  }

  if(have_partition)
  {
    printf("mkminixfs: device = %s, partition = %lu\n", device, partition);
  }
  else
  {
    printf("mkminixfs: device = %s\n", device);
  }


  printf("mkminixfs: opening device %s\n", device);
  FILE* image_fd = fopen(device, "r+b");

  if (image_fd == 0)
  {
    fprintf(stderr, "mkminixfs: Error opening %s\n", device);
    return -1;
  }

  /*
    char* end;
    //size_t partition = strtoul(argv[2], &end, 10);
    //printf("Partition number: %zu\n", partition);
    if (strlen(end) != 0)
    {
    fclose(image_fd);
    printf("partition has to be a number!\n");
    return -1;
    }
  */
  if(have_partition && ((partition < 1) || (4 < partition)))
  {
    fclose(image_fd);
    fprintf(stderr, "mkminixfs: Error, partition number has to be in range [1-4]\n");
    return -1;
  }


  size_t part_byte_offset = 0;
  size_t part_byte_size = 0;


  if(have_partition)
  {
    MBR image_mbr;

    fseek(image_fd, 0, SEEK_SET);
    size_t mbr_read_count =  fread(&image_mbr, sizeof(image_mbr), 1, image_fd);
    if(mbr_read_count != 1)
    {
      if(ferror(image_fd))
      {
        fprintf(stderr, "mkminixfs: Error while reading MBR from %s\n", argv[1]);
      }
      if(feof(image_fd))
      {
        fprintf(stderr, "mkminixfs: %s EOF reached while reading MBR\n", argv[1]);
      }
      fprintf(stderr, "mkminixfs: was not able to read the disk image MBR\n");
      fclose(image_fd);
      return -1;
    }

    if(image_mbr.signature != 0xAA55)
    {
      printf("mkminixfs: Warning, disk MBR not marked as valid boot sector\n");
    }

    FP* part_table = (FP*)&image_mbr.parts;
    for(size_t i = 0; i < 4; ++i)
    {
      printf("mkminixfs: Partition %zu, sectors [%u -> %u), bytes [%zu -> %zu), type: %x, bootable: %u\n",
             i + 1,
             part_table[i].relsect,
             part_table[i].relsect + part_table[i].numsect,
             (size_t)part_table[i].relsect * 512,
             (size_t)(part_table[i].relsect + part_table[i].numsect) * 512,
             part_table[i].systid,
             part_table[i].bootid != 0);
    }

    if(part_table[partition - 1].systid == 0)
    {
      fprintf(stderr, "mkminixfs: No partition %lu on image\n", partition);
      fclose(image_fd);
      return -1;
    }

    if(part_table[partition - 1].systid != 0x81)
    {
      printf("mkminixfs: Warning, partition type 0x%x != minixfs (0x81)\n", part_table[partition - 1].systid);
    }

    part_byte_offset = (size_t)part_table[partition - 1].relsect * 512;
    part_byte_size = (size_t)part_table[partition - 1].numsect * 512;
  }
  else
  {
    fseek(image_fd, 0, SEEK_END);
    part_byte_size = ftell(image_fd);
    part_byte_offset = 0;
  }

  size_t num_blocks = part_byte_size / BLOCK_SIZE;
  printf("mkminixfs: num 1024 byte blocks in partition: %zu\n", num_blocks);

  fseek(image_fd, part_byte_offset, SEEK_SET);
  char null_block[512];
  memset(null_block, 0, sizeof(null_block));
  for(size_t i = 0; i < part_byte_size; i += sizeof(null_block))
  {
    assert(fwrite(null_block, sizeof(null_block), 1, image_fd) == 1);
  }

  size_t num_remaining_blocks = num_blocks - 2;
  size_t remaining_bytes = num_remaining_blocks * BLOCK_SIZE;
  size_t num_inodes = (remaining_bytes / 100) / sizeof(MinixFSInode::MinixFSInodeOnDiskDataV3);
  size_t num_inode_bm_blocks = ((num_inodes / 8) / BLOCK_SIZE) + (((num_inodes / 8) % BLOCK_SIZE) != 0);
  size_t num_inode_table_blocks = ((num_inodes * sizeof(MinixFSInode::MinixFSInodeOnDiskDataV3)) / BLOCK_SIZE) + (((num_inodes * sizeof(MinixFSInode::MinixFSInodeOnDiskDataV3)) % BLOCK_SIZE) != 0);
  printf("mkminixfs: Num inodes: %zu, num inode bm blocks: %zu, inode table blocks: %zu\n", num_inodes, num_inode_bm_blocks, num_inode_table_blocks);

  size_t blocks_for_zones = num_blocks - 2 - num_inode_bm_blocks - num_inode_table_blocks;
  size_t max_num_zones = ((blocks_for_zones * BLOCK_SIZE) / ZONE_SIZE);
  size_t num_zone_bm_blocks = ((max_num_zones / 8) / BLOCK_SIZE) + (((max_num_zones / 8) % BLOCK_SIZE) != 0);
  printf("mkminixfs: Remaining blocks for zones + zone bm: %zu\n", blocks_for_zones);
  printf("mkminixfs: Num zone bm blocks: %zu\n", num_zone_bm_blocks);

  size_t remaining_blocks_for_zones = num_blocks - 2 - num_inode_bm_blocks - num_zone_bm_blocks - num_inode_table_blocks;
  size_t num_data_zones = (remaining_blocks_for_zones * BLOCK_SIZE) / ZONE_SIZE;
  printf("mkminixfs: Remaining blocks for zones: %zu, num data zones: %zu\n", remaining_blocks_for_zones, num_data_zones);

  MinixFSSuperblock::MinixFSSuperblockOnDiskDataV3 sb_data{};
  sb_data.s_magic = MINIX_V3;
  sb_data.s_blocksize = 1024;
  sb_data.s_log_zone_size = 0;
  sb_data.s_num_inodes = num_inodes;
  sb_data.s_imap_blocks = num_inode_bm_blocks;
  sb_data.s_zmap_blocks = num_zone_bm_blocks;
  sb_data.s_num_zones = num_blocks;
  sb_data.s_firstdatazone = 2 + num_inode_bm_blocks + num_zone_bm_blocks + num_inode_table_blocks;
  sb_data.s_max_file_size = (7 + 1*(BLOCK_SIZE/4) + 1*(BLOCK_SIZE/4)*(BLOCK_SIZE/4))*BLOCK_SIZE; // TODO: Is this correct for minixfsv3 ?


  fseek(image_fd, part_byte_offset + BLOCK_SIZE, SEEK_SET);
  assert(fwrite(&sb_data, sizeof(sb_data), 1, image_fd) == 1);

  // Set bits 0 + 1 in inode bitmap (0 = reserved, 1 = root inode)
  size_t inode_bm_byte_offset = 2 * BLOCK_SIZE;
  fseek(image_fd, part_byte_offset + inode_bm_byte_offset, SEEK_SET);
  uint8 inode_bm[1];
  inode_bm[0] = 0x03;
  assert(fwrite(&inode_bm, sizeof(inode_bm), 1, image_fd) == 1);

  size_t inode_table_byte_offset = (2 + num_inode_bm_blocks + num_zone_bm_blocks) * BLOCK_SIZE;
  fseek(image_fd, part_byte_offset + inode_table_byte_offset, SEEK_SET);
  MinixFSInode::MinixFSInodeOnDiskDataV3 inode_tbl[2]{};
  inode_tbl[0].i_mode = 0x41ed; // directory, user: RWX, group: R-X, others: R-X
  assert(fwrite(&inode_tbl, sizeof(inode_tbl), 1, image_fd) == 1);


  {
    printf("mkminixfs: Loading MinixFSSuperblock\n");
    MinixFSSuperblock superblock(0, (size_t)image_fd, part_byte_offset);
  }

  if(have_partition)
  {
    printf("mkminixfs: Created minixfs v3 file system on device: %s, partition: %lu\n", device, partition);
  }
  else
  {
    printf("mkminixfs: Created minixfs v3 file system on device: %s\n", device);
  }

  fclose(image_fd);
  return 0;
}

#endif
