//
//
// CVS Log Info for $RCSfile: PathWalker.h,v $
//
// $Id: PathWalker.h,v 1.3 2005/09/02 17:57:58 davrieb Exp $
// $Log: PathWalker.h,v $
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
class Mount;

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
  LAST_NORM,
  LAST_ROOT,
  /// The last component is the root directory
  LAST_DOT,
  LAST_DOTDOT,
  LAST_BIND
};

/// Error Codes for the path walker
enum {
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
  Mount* mount_;

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

  int32 init(const char* name, uint32 flags);

protected:


  /// extract the first part of a path
  ///
  /// @param path is a char* cantaining the path to get the next part from.
  /// @param npart will be filled with a newly allocated copy of the next part.
  /// @return is the start position of the extracted part in path.
  ///         It is empty if there is no next part
  ///         In case of an error a null pointer is returned.
  ///
  int32 getNextPart(const char* path, char *npart);

  /// Skip any leading slashes on path.
  ///
  /// @return is a pointer to the first charachter that is not a '/'.
  char *skipSeparator(char const *path) const;

};

#endif

