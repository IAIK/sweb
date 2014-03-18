/**
 * @file a collection of all file system error codes returned by the kernel's
 * file system calls (VfsSyscall and specific file systems)
 * @filename FsErrorCodes.h
 *
 * Created on: 07.08.2012
 * @author: Christopher Walles
 */

#ifndef FSERRORCODES_H_
#define FSERRORCODES_H_

// successful operation
#define FS_OPERATION_SUCCESSFUL           (0)

/**
 * general errors
 */
#define FS_ERROR_INVALID_ARGUMENT          (-1)

#define FS_ERROR_PATH_TOO_LONG             (-4)
#define FS_ERROR_FILENAME_TOO_LONG         (-5)

#define FS_ERROR_WRITE_PERMISSION_DENIED   (-6)

/**
 * path resolution errors
 */
// most general
#define FS_ERROR_FAILED_TO_RESOLVE_PATH    (-10)
#define FS_ERROR_SEARCH_PERMISSION_DENIED  (-11)
#define FS_ERROR_PATH_PART_IS_NOT_DIR      (-12) // a part of the path does not name a Directory

/**
 * Directory Syscall specific
 */
#define FS_ERROR_DIR_NOT_EMPTY             (-20)

#endif /* FSERRORCODES_H_ */
