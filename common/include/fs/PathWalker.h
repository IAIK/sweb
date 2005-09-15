// Projectname: SWEB
// Simple operating system for educational purposes

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

//-------------------------------------------------------------------------
/**
 * PathWalker
 * 
 * this class illustrate how the VFS derives an inode from the corresponding
 * file pathname. Pathname lookup is performed by three methods: pathInit(),
 * pathWalk() and pathRelease().
 */ 
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
  /// @param pathname A pointer to the file pathname to be resolved
  /// @param flags The vlaue of flags that represent how to look-up file is going
  ///        to be accessed
  /// @return On success, it is returned 0. On error, it return a non-Null value.
  int32 pathInit(const char* pathname, uint32 flags);
  
  /// this method takes care of the lookup operation and stores the pointers
  /// to the dentry_ object and mounted filesystem object relative to the last
  /// component of the pathname.
  /// @param pathname A pointer to the file pathname to be resolved
  /// @return On success, it is returned 0. On error, it return a non-Null value.
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
  char* getNextPart(const char* path, int32 &npart_len);
  
  /// Skip any leading slashes on path.
  ///
  /// @return is a pointer to the first charachter that is not a '/'.
  char *skipSeparator(char const */*path*/) const;

};

#endif

