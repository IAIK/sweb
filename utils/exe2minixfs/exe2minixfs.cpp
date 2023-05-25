#ifdef EXE2MINIXFS
#include "types.h"
#include <sys/stat.h>
#if _POSIX_C_SOURCE >= 1 || _XOPEN_SOURCE || _POSIX_SOURCE
  #include <sys/file.h>
#endif
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

#include "Dentry.h"
#include "FileSystemInfo.h"
#include "Superblock.h"
#include "MinixFSType.h"
#include "MinixFSSuperblock.h"
#include "VfsSyscall.h"
#include "VfsMount.h"

Superblock* superblock_;
FileSystemInfo* default_working_dir;

constexpr size_t SECTOR_SIZE = 512;

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

// obviously NOT atomic, we need this for compatibility in single threaded host code
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
    printf("exe2minixfs: Error opening %s\n", argv[1]);
    return -1;
  }

  char* end;
  size_t partition = strtoul(argv[2], &end, 10);
  //printf("Partition number: %zu\n", partition);
  if (strlen(end) != 0)
  {
    fclose(image_fd);
    printf("exe2minixfs: partition has to be a number!\n");
    return -1;
  }
  if(partition > 4)
  {
    fclose(image_fd);
    printf("exe2minixfs: Partition number has to be either 0 (use whole device/file) or in range [1-4]\n");
    return -1;
  }

#if _POSIX_C_SOURCE >= 1 || _XOPEN_SOURCE || _POSIX_SOURCE
  int posix_fd = fileno(image_fd);
  int result = flock(posix_fd, LOCK_EX);
  if (result)
  {
      printf("Unable to get file lock for %s\n", argv[1]);
      return -1;
  }
#endif

  size_t part_byte_offset = 0;
  size_t part_byte_size = 0;

  if(partition != 0)
  {
    MBR image_mbr;

    fseek(image_fd, 0, SEEK_SET);
    size_t mbr_read_count =  fread(&image_mbr, sizeof(image_mbr), 1, image_fd);
    if(mbr_read_count != 1)
    {
      if(ferror(image_fd))
      {
        printf("exe2minixfs: Error while reading MBR from %s\n", argv[1]);
      }
      if(feof(image_fd))
      {
        printf("exe2minixfs: %s EOF reached while reading MBR\n", argv[1]);
      }
      printf("exe2minixfs: unable to read the disk image MBR\n");
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
      printf("exe2minixfs: No partition %zu on image\n", partition);
      fclose(image_fd);
      return -1;
    }

    if((part_table[partition - 1].systid != 0x80) && (part_table[partition - 1].systid != 0x81))
    {
      printf("exe2minixfs: Warning, partition type 0x%x != minixfs (0x80/0x81)\n", part_table[partition - 1].systid);
    }

    part_byte_offset = (size_t)part_table[partition - 1].relsect * SECTOR_SIZE;
    part_byte_size = part_table[partition - 1].numsect * SECTOR_SIZE;
  }

  MinixFSType* minixfs_type = new MinixFSType();

  superblock_ = (Superblock*) new MinixFSSuperblock(minixfs_type, (size_t)image_fd, part_byte_offset, part_byte_size);
  Dentry *root = superblock_->getRoot();
  superblock_->setMountPoint(root);
  Dentry *mount_point = superblock_->getMountPoint();
  mount_point->setMountedRoot(mount_point);

  VfsMount vfs_dummy_(nullptr, mount_point, root, superblock_, 0);


  default_working_dir = new FileSystemInfo();
  Path root_path(root, &vfs_dummy_);
  default_working_dir->setRoot(root_path);
  default_working_dir->setPwd(root_path);

  for (int32 i = 2; i <= argc / 2; i++)
  {
    FILE* src_file = fopen(argv[2 * i - 1], "rb");

    if (src_file == 0)
    {
      printf("exe2minixfs: Failed to open host file %s\n", argv[2 * i - 1]);
      break;
    }

    fseek(src_file, 0, SEEK_END);
    size_t size = ftell(src_file);

    char *buf = new char[size];

    fseek(src_file, 0, SEEK_SET);
    assert(fread(buf, 1, size, src_file) == size && "exe2minixfs: fread was not able to read all bytes of the file");
    fclose(src_file);

    VfsSyscall::rm(argv[2 * i]);

    eastl::string pathname(argv[2 * i]);
    size_t next_slash = pathname.find('/');
    while(next_slash != eastl::string::npos)
    {
      if(next_slash != 0)
      {
        eastl::string dir_path(pathname.substr(0, next_slash));
        VfsSyscall::mkdir(dir_path.c_str(), 0);
      }
      next_slash = pathname.find('/', next_slash + 1); // TODO: Proper normalization required. This will fail for edge cases
    }

    int32 fd = VfsSyscall::open(argv[2 * i], 4 | 8); // i.e. O_RDWR | O_CREAT
    if (fd < 0)
    {
      printf("exe2minixfs: Failed to open SWEB file %s\n", argv[2 * i]);
      delete[] buf;
      //continue;
      delete default_working_dir;
      delete superblock_;
      fclose(image_fd);
      return -1;

    }
    int32 write_status = VfsSyscall::write(fd, buf, size);
    if((size_t)write_status != size)
    {
      printf("exe2minixfs: Writing %s failed with retval %d (expected %zu)\n", argv[2 * i], write_status, size);
    }
    VfsSyscall::close(fd);

    delete[] buf;
  }

  delete default_working_dir;
  delete superblock_;
  delete minixfs_type;
  fclose(image_fd);

  return 0;
}

#endif
