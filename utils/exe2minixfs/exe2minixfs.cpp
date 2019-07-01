#ifdef EXE2MINIXFS
#include "types.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

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
  if (argc < 3 || argc % 2 == 0)
  {
    printf("Syntax: %s <filename of minixfs-formatted image> <partition number> [file1-src file1-dest [file2-src file2-dest [....]]]\n",
           argv[0]);
    return -1;
  }

  //printf("Exe2minixfs opening disk image %s\n", argv[1]);
  FILE* image_fd = fopen(argv[1], "r+b");

  if (image_fd == 0)
  {
    printf("Error opening %s\n", argv[1]);
    return -1;
  }

  char* end;
  size_t partition = strtoul(argv[2],&end,10);
  //printf("Partition number: %zu\n", partition);
  if (strlen(end) != 0)
  {
    fclose(image_fd);
    printf("partition has to be a number!\n");
    return -1;
  }
  if(partition > 4)
  {
    fclose(image_fd);
    printf("Partition number has to be either 0 (use whole device/file) or in range [1-4]\n");
    return -1;
  }

  size_t part_byte_offset = 0;

  if(partition != 0)
  {

    MBR image_mbr;

    fseek(image_fd, 0, SEEK_SET);
    size_t mbr_read_count =  fread(&image_mbr, sizeof(image_mbr), 1, image_fd);
    if(mbr_read_count != 1)
    {
      if(ferror(image_fd))
      {
        printf("Error while reading MBR from %s\n", argv[1]);
      }
      if(feof(image_fd))
      {
        printf("%s EOF reached while reading MBR\n", argv[1]);
      }
      printf("exe2minixfs was not able to read the disk image MBR\n");
      fclose(image_fd);
      return -1;
    }

    if(image_mbr.signature != 0xAA55)
    {
      printf("exe2minixfs: Warning, disk MBR not marked as valid boot sector\n");
    }

    FP* part_table = (FP*)&image_mbr.parts;

    if(part_table[partition - 1].systid == 0)
    {
      printf("No partition %zu on image\n", partition);
      fclose(image_fd);
      return -1;
    }

    if((part_table[partition - 1].systid != 0x80) && (part_table[partition - 1].systid != 0x81))
    {
      printf("exe2minixfs: Warning, partition type 0x%x != minixfs (0x80/0x81)\n", part_table[partition - 1].systid);
    }

    part_byte_offset = (size_t)part_table[partition - 1].relsect * 512;
  }
  else
  {
    part_byte_offset = 0;
  }

  superblock_ = (Superblock*) new MinixFSSuperblock(0, (size_t)image_fd, part_byte_offset);
  //printf("exe2minxfs: Created superblock\n");
  Dentry *mount_point = superblock_->getMountPoint();
  //printf("exe2minxfs: Got mount point\n");
  mount_point->setMountPoint(mount_point);
  Dentry *root = superblock_->getRoot();


  default_working_dir = new FileSystemInfo();
  default_working_dir->setFsRoot(root, &vfs_dummy_);
  default_working_dir->setFsPwd(root, &vfs_dummy_);

  //printf("exe2minxfs: Created working dir\n");

  for (int32 i = 2; i <= argc / 2; i++)
  {
    FILE* src_file = fopen(argv[2 * i - 1], "rb");

    if (src_file == 0)
    {
      printf("Wasn't able to open file %s\n", argv[2 * i - 1]);
      break;
    }

    fseek(src_file, 0, SEEK_END);
    size_t size = ftell(src_file);

    char *buf = new char[size];

    fseek(src_file, 0, SEEK_SET);
    assert(fread(buf, 1, size, src_file) == size && "fread was not able to read all bytes of the file");
    fclose(src_file);

    VfsSyscall::rm(argv[2 * i]);
    //printf("Open path: %s\n", argv[2 * i]);

    ustl::string pathname(argv[2 * i]);
    size_t next_slash = pathname.find('/');
    while(next_slash != ustl::string::npos)
    {
      //printf("pathname: %s, next slash at: %zu\n", pathname.c_str(), next_slash);
      if(next_slash != 0)
      {
        ustl::string dir_path(pathname.substr(0, next_slash));
        //printf("Attempting to create path %s\n", dir_path.c_str());
        VfsSyscall::mkdir(dir_path.c_str(), 0);
      }
      next_slash = pathname.find('/', next_slash + 1); // TODO: Proper normalization required. This will fail for edge cases
    }

    int32 fd = VfsSyscall::open(argv[2 * i], 2 | 4);
    if (fd < 0)
    {
      printf("no success\n");
      delete[] buf;
      //continue;
      delete default_working_dir;
      delete superblock_;
      fclose(image_fd);
      return -1;

    }
    VfsSyscall::write(fd, buf, size);
    VfsSyscall::close(fd);

    delete[] buf;
  }
  delete default_working_dir;
  delete superblock_;
  fclose(image_fd);
  return 0;
}

#endif
