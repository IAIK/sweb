/**
 * Filename: Thread.h
 * Description:
 *
 * Created on: 02.09.2012
 * Author: chris
 */

#ifndef THREAD_H_
#define THREAD_H_

class FsWorkingDirectory;

/**
 * @class this is a dummy thread class for the FS
 */
class Thread
{
public:
  Thread();
  Thread(FsWorkingDirectory* wd, const char* name = "");

  /**
   * Destructor
   */
  virtual ~Thread();

  /**
   * getting the informations about the working Directory of this
   * Thread
   * @return the thread's FsWorkingDirectory
   */
  FsWorkingDirectory* getWorkingDirInfo(void);

  /**
   * sets the working directory informations of the this Thread
   * @param working_dir the new working directory informations
   */
  void setWorkingDirInfo(FsWorkingDirectory* working_dir);

  /**
   * pseudo run-method
   */
  virtual void Run(void);

private:

  //
  FsWorkingDirectory* working_dir_;
};

#endif /* THREAD_H_ */
