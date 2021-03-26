#pragma once

#include "types.h"

class Dentry;
class VfsMount;
class Path;

#define MAX_NAME_LEN 100

/**
 * If the last component is a symbolic link follow it
 */
#define LOOKUP_FOLLOW     0x0001

/**
 * If this flag is set the last component must be a directory
 */
#define LOOKUP_DIRECTORY  0x0002

/**
 * If this flag is set the last component of the pathname must exist
 */
#define LOOKUP_POSITICE   0x0004

/**
 * If this flag is set lookup the directory including the last component
 */
#define LOOKUP_PARENT     0x0008

/**
 * @enum Type of the last component on LOOKUP_PARENT
 */
enum
{
  /**
   * The last component is a regular filename
   */
  LAST_NORM,

  /**
   * The last component is the root directory
   */
  LAST_ROOT,

  /**
   * The last component is "."
   */
  LAST_DOT,

  /**
   * The last component is ".."
   */
  LAST_DOTDOT,

  /**
   * The last component is a symbolic link into a special filesystem
   */
  LAST_BIND
};

/**
 * @enum Error Codes for the path walk
 */
enum
{
  PW_SUCCESS = 0,
  /**
   * The path was not found
   */
  PW_ENOTFOUND,
  /**
   * The path to look up is invalid
   */
  PW_EINVALID
};

class PathWalker
{
  public:

    /**
     * check the first character of the path (begins with '/' or
     * with pwd). Initialize the flags_.
     * takes care of the lookup operation and stores the pointers
     * to the dentry_ object and mounted filesystem object relative to the last
     * component of the pathname.
     * @param pathname A pointer to the file pathname to be resolved
     * @param start Start directory of the file system walk
     * @param root Root directory for the file system walk
     * @param flags The value of flags that represent how to look-up file is going
     *         to be accessed
     * @param out Output parameter: Found file path
     * @return Returns 0 on success, != 0 on error
     */
    static int32 pathWalk(const char* pathname, const Path& pwd, const Path& root, uint32 flags_ __attribute__ ((unused)), Path& out, Path* parent = nullptr);

  protected:

    /**
     * extract the first part of a path
     * @param path is a char* containing the path to get the next part from.
     * @param npart_len will be set to the length of the next part
     * @return length of the next part
     */
    static int32 getNextPartLen(const char* path, int32& npart_len);
  private:
    PathWalker();
    ~PathWalker();
};


