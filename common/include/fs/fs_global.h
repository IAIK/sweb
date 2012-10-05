/**
 * @file fs_global.h
 */

#ifndef FS_GLOBAL_H_
#define FS_GLOBAL_H_

#include "fs/FileDescriptor.h"

/**
 * the global file descriptor list
 */
extern ustl::list<FileDescriptor*> global_fd;

#endif /* FS_GLOBAL_H_ */
