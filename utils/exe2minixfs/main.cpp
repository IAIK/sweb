#include "types.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdio>
#include <unistd.h>

#include <string>
#include <iostream>
#include <sstream>
#include <stdlib.h>

#include "Dentry.h"
#include "FileSystemInfo.h"
#include "Superblock.h"
#include "MinixFSSuperblock.h"
#include "VfsSyscall.h"

Superblock* superblock_;
FileSystemInfo* fs_info;
VfsMount vfs_dummy_;

int main(int argc, char *argv[])
{
  if (argc < 3 || argc % 2 == 0)
  {
    std::cout << "Syntax:" << argv[0] << "<filename of minixfs-formatted image> <offset in bytes> [file1-src file1-dest"
        << " [file2-src file2-dest [....]]]" << std::endl << std::endl;
    return -1;
  }

  int32 image_fd = open(argv[1], O_RDWR);

  if (image_fd < 0)
  {
    std::cout << "Error opening " << argv[1] << std::endl;
    return -1;
  }

  std::istringstream stream(argv[2]);
  uint64 offset;
  stream >> offset;
  if (stream.fail() || !stream.eof())
  {
    close(image_fd);
    std::cout << "offset has to be a number!" << std::endl;
    return -1;
  }

  superblock_ = (Superblock*) new MinixFSSuperblock(0, image_fd, offset);
  Dentry *mount_point = superblock_->getMountPoint();
  mount_point->setMountPoint(mount_point);
  Dentry *root = superblock_->getRoot();

  fs_info = new FileSystemInfo();
  fs_info->setFsRoot(root, &vfs_dummy_);
  fs_info->setFsPwd(root, &vfs_dummy_);

  for (int32 i = 2; i <= argc / 2; i++)
  {
    int32 src_file = open(argv[2 * i - 1], O_RDONLY);

    if (src_file < 0)
    {
      std::cout << "Wasn't able to open file " << argv[2 * i - 1] << std::endl;
      break;
    }

    size_t size = lseek(src_file, 0, SEEK_END);

    char *buf = new char[size];

    lseek(src_file, 0, SEEK_SET);
    read(src_file, buf, size);
    close(src_file);

    VfsSyscall::rm(argv[2 * i]);
    int32 fd = VfsSyscall::open(argv[2 * i], 2 | 4); // O_RDWR | O_CREAT
    if (fd < 0)
    {
      std::cout << "no success" << std::endl;
      delete[] buf;
      continue;
    }
    VfsSyscall::write(fd, buf, size);
    VfsSyscall::close(fd);

    delete[] buf;
  }
  delete fs_info;
  delete superblock_;
  close(image_fd);
  return 0;
}

