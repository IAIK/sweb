/**
 * Filename: debug_print.cpp
 * Description:
 *
 * Created on: 02.09.2012
 * Author: chris
 */

#include "types.h"
#include "debug_print.h"

#include <iostream>

#include <stdio.h>
#include <stdarg.h>

void debug(uint32_t flag, const char *fmt, ... )
{
  va_list args;
  va_start ( args, fmt );

  if ( flag & OUTPUT_ENABLED )
  {
    switch ( flag )
    {
      case CACHE:
        std::cout << "[CACHE      ]";
        break;
      case READ_CACHE:
        std::cout << "[READ_CACHE ]";
        break;
      case WRITE_CACHE:
        std::cout << "[WRITE_CACHE]";
        break;

      case VFSSYSCALL:
        std::cout << "[VFSSYSCALL ]";
        break;
      case FILE_SYSTEM:
        std::cout << "[FILE_SYSTEM]";
        break;
      case VOLUME_MANAGER:
        std::cout << "[VOLUME_MAN ]";
        break;
      case FS_DEVICE:
        std::cout << "[FS_DEVICE  ]";
        break;
      case FS_BITMAP:
        std::cout << "[FS_BITMAP  ]";
        break;
      case FS_INODE:
        std::cout << "[INODE      ]";
        break;
      case FS_UTIL:
        std::cout << "[FS_UTIL    ]";
        break;
      case FS_TESTCASE:
        std::cout << "[FS_TESTCASE]";
        break;

      case FS_UNIX:
        std::cout << "[FS_UNIX    ]";
        break;
      case INODE_TABLE:
        std::cout << "[INODE_TABLE]";
        break;

      case FS_MINIX:
        std::cout << "[MINIX      ]";
        break;
    }

    char* str = new char[1024];
    vsprintf (str, fmt, args );
    std::cout << str;

    delete[] str;
  }

  va_end ( args );
}

void kprintfd ( const char *fmt, ... )
{
  va_list args;
  va_start ( args, fmt );

  char* str = new char[1024];
  vsprintf (str, fmt, args );
  std::cout << str;

  delete[] str;

  va_end ( args );
}
