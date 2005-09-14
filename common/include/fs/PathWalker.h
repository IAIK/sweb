//
//
// CVS Log Info for $RCSfile: PathWalker.h,v $
//
// $Id: PathWalker.h,v 1.6 2005/09/14 14:22:16 davrieb Exp $
// $Log: PathWalker.h,v $
// Revision 1.5  2005/09/12 17:55:53  qiangchen
// test the VFS (vfsvfs__syscall)
//
// Revision 1.4  2005/09/10 19:25:27  qiangchen
//  21:24:09 up 14:16,  3 users,  load average: 0.08, 0.09, 0.14
// USER     TTY      FROM              LOGIN@   IDLE   JCPU   PCPU WHAT
// chen     :0       -                12:11   ?xdm?   1:01m  1.35s /usr/bin/gnome-
// chen     pts/0    :0.0             12:15    1.00s  0.34s  0.03s cvs commit
// chen     pts/1    :0.0             12:33    5:23m  3.13s  0.04s -bash
//
// Revision 1.3  2005/09/02 17:57:58  davrieb
// preparations to  build a standalone filesystem testsuite
//
// Revision 1.2  2005/09/01 17:07:49  davrieb
// update path walking
//
// Revision 1.1  2005/08/11 16:46:57  davrieb
// add PathWalker
//
//
//

#ifndef PathWalker_h__
#define PathWalker_h__


#include "types.h"

// forward declarations
class Dentry;
class VfsMount;

#define MAX_NAME_LEN 100

/// If the last component is a symbolic link follow it
#define LOOKUP_FOLLOW     0x0001

/// If this flag is set the last component must be a directory
#define LOOKUP_DIRECTORY  0x0002

/// If this flag is set the last component of the pathname must exist
#define LOOKUP_POSITICE   0x0004

/// If this flag is set lookup the directory including the last component
#define LOOKUP_PARENT     0x0008


/// Type of the last component on LOOKUP_PARENT
enum {
  /// The last component is a regular filename
  LAST_NORM, 

  /// The last component is the root directory
  LAST_ROOT,
  
  /// The last component is "."
  LAST_DOT,
  
  /// The last component is ".."
  LAST_DOTDOT,
  
  /// The last component is a symbolic link into a special filesystem
  LAST_BIND
};

/// Error Codes for the path init
enum
{
  PI_SUCCESS = 0,
  /// The path was not found
  PI_ENOTFOUND
};

/// Error Codes for the path walk
enum 
{
  PW_SUCCESS = 0,
  /// The path was not found
  PW_ENOTFOUND,
  /// The path to look up is invalid
  PW_EINVALID
};


/// The maximal length of a filename
#define MAX_NAME_LENGTH 4096

class PathWalker
{

protected:

  /// The resluting Dentry after the lookup has succeded
  Dentry *dentry_;

  /// The Mount the path is located in
  VfsMount* vfs_mount_;

  /// The lookup flags
  int32 flags_;

  /// Flag indicating the type of the last path component.
  int32 last_type_;

  /// The last path component
  char* last_;

public:

  /// The Constructor
  PathWalker();

  /// The destructor
  ~PathWalker();

  /// this method check the first character of the path (begins with '/' or 
  /// with pwd). Initialize the flags_.
  int32 pathInit(const char* pathname, uint32 flags);
  
  /// this method takes care of the lookup operation and stores the pointers
  /// to the dentry_ object and mounted filesystem object relative to the last
  /// component of the pathname.
  int32 pathWalk(const char* pathname);
  
  /// this method terminate the pathname lookup of the mount point.
  void pathRelease();

  Dentry* getDentry() { return dentry_; }
  VfsMount* getVfsMount() { return vfs_mount_; }

protected:


  /// extract the first part of a path
  ///
  /// @param path is a char* cantaining the path to get the next part from.
  /// @param npart will be filled with a newly allocated copy of the next part.
  /// @return is the start position of the extracted part in path.
  ///         It is empty if there is no next part
  ///         In case of an error a null pointer is returned.
  ///
  char* getNextPart(const char* path, int32 &npart_len);
  
  /// Skip any leading slashes on path.
  ///
  /// @return is a pointer to the first charachter that is not a '/'.
  char *skipSeparator(char const *path) const;

};

#endif

