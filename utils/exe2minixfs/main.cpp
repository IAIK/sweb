
#include "MinixFSTypes.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdio>
#include <unistd.h>

#include <string>
#include <iostream>
#include <sstream>
#include <stdlib.h>

#include "VfsSyscall.h"

int main (int argc, char *argv[])
{
  if (argc < 3 || argc % 2 == 0)
  {
    std::cout << "Syntax:" << argv[0] << "<filename of minixfs-formatted image> <offset in bytes> [file1-src file1-dest"
              << " [file2-src file2-dest [....]]]" << std::endl << std::endl;
    return -1;
  }

  int32 image_fd = open(argv[1],  O_RDWR);

  if (image_fd < 0)
  {
    std::cout << "Error opening " << argv[1] << std::endl;
    return -1;
  }

  try
  {
    std::istringstream stream(argv[2]);
    uint64 asdf;
    stream >> asdf;
    if(stream.fail() || !stream.eof())
    {
      close(image_fd);
      std::cout << "offset has to be a number!" << std::endl;
      return -1;
    }

    VfsSyscall vfs_syscall(image_fd, asdf);

    for(int32 i=2; i <= argc/2; i++)
    {
      int32 src_file = open(argv[2*i-1], O_RDONLY);
  
      if(src_file < 0)
      {
        std::cout << "Wasn't able to open file " << argv[2*i-1] << std::endl;
        break;
      }
  
      size_t size = lseek(src_file, 0, SEEK_END);
  
      char *buf = new char[size];
  
      lseek(src_file, 0, SEEK_SET);  
      read(src_file, buf, size);
      close(src_file);

      vfs_syscall.rm(argv[2*i]);
      int32 fd = vfs_syscall.open(argv[2*i], O_RDWR);
      if(fd < 0)
      {
        delete[] buf;
        continue;
      }
      vfs_syscall.write(fd, buf, size);
      vfs_syscall.close(fd);

      delete[] buf;
    }
  }
  catch(std::bad_alloc const &exc)
  {
    std::cout << "std::bad_alloc - exception caught: " << exc.what() << std::endl;
    close(image_fd);
    return -1;
  }
  catch(...)
  {
    std::cout << "caught unknown exception in main()!" << std::endl;
    close(image_fd);
    return -1;
  }

  close(image_fd);
  return 0;
}

