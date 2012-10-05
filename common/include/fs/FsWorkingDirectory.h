/**
 * Filename: FsWorkingDirectory.h
 * Description:
 *
 * Created on: 09.07.2012
 * Author: Christopher Walles
 */

#ifndef FSWORKINGDIRECTORY_H_
#define FSWORKINGDIRECTORY_H_

#include "types.h"

class Directory;

/**
 * @class FsWorkingDirectory holds the working Directory Informations
 * of a Thread / Process
 */
class FsWorkingDirectory
{
public:
  /**
   * constructor
   */
  FsWorkingDirectory();

  /**
   * copy constructor
   * @param object to copy
   */
  FsWorkingDirectory(const FsWorkingDirectory& cpy);

  /**
   * destructor
   */
  virtual ~FsWorkingDirectory();

  /**
   * sets the current working directory
   * @param working_dir the new working directory
   * @return error-code
   */
  int32 setWorkingDir(const char* working_dir);

  /**
   * getting the current working directory
   * @return the working directory as char* or a ptr to the Directory-object
   */
  //char* getWorkingDirPath(void) const;
  const char* getWorkingDirPath(void) const;
  //Directory* getWorkingDir(void);

  /**
   * sets the new root directory of the Thread / Process
   * @param new_root_dir the new root directory
   */
  void changeRootDir(const char* new_root_dir);

  /**
   * getting the root-directory
   * @return
   */
  //char* getRootDirPath(void) const;
  const char* getRootDirPath(void) const;
  //Directory* getRootDir(void);

private:

  void deleteRootString(void);
  void deleteWorkingString(void);

  // the absolute path to the working-directory
  char* working_dir_path_;

  // the Directory (I-Node) of the current Directory
  Directory* working_dir_;

  // the customized root directory of the Thread
  char* root_dir_path_;

  // root-directory i-node
  Directory* root_dir_;
};

#endif /* FSWORKINGDIRECTORY_H_ */
