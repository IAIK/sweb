/**
 * Filename: ImageUtil.h
 * Description:
 *
 * Created on: 01.09.2012
 * Author: chris
 */

#ifndef IMAGEUTIL_H_
#define IMAGEUTIL_H_

#define USE_FILE_SYSTEM_ON_GUEST_OS             1

#include <string>
#include <vector>
class UtilTask;
class ImageInfo;

/**
 * @class Program management class
 */
class Program
{
public:
  /**
   * constructor
   *
   * @param args
   */
  Program(std::vector<std::string>& args);

  /**
   * destructor
   */
  virtual ~Program();

  /**
   * runs the Program
   *
   * @return true / false
   */
  bool run(void);

  /**
   * getting the current ImageInfo object
   * @return ImageInfo
   */
  ImageInfo* getImageInfo(void);

  /**
   * setting a new image info
   *
   * @param info NOTE: ImageUtil will free memory after usage!
   */
  void setImageInfo(ImageInfo* info);

  /**
   * getting the number of passed arguments
   *
   * @return the number of arguments
   */
  unsigned int getNumArgs(void) const;

  /**
   * getting the n-th argument
   * @return the n-th argument
   */
  //const char* getArg(unsigned int arg_nr) const;
  std::string getArg(unsigned int arg_nr) const;

  /**
   * getting the current used partition (specified in parameter 3)
   */
  int getCurrentUsedPartition(void) const;

  /**
   * prints all currently registered tasks
   */
  void printTaskInfo(void) const;

private:

  /**
   * inits
   */
  void init(void);

  /**
   * creates one instance of each available tasks and inserts it
   * into the given vector
   */
  void createTasks(void);

  /**
   * finds an appropriate task in the list
   *
   * @param option_name char name of the option
   * @return the task or NULL
   */
  UtilTask* findTask(char option_name);

  /**
   * deletes all tasks
   */
  void deleteTasks(void);

  // the arguments that were passed the Program
  std::vector<std::string> args_;

  // a set of taks
  std::vector<UtilTask*> tasks_;

  // current ImageInfo
  ImageInfo* image_info_;

  // current partition
  int cur_partition_;
};

#endif /* IMAGEUTIL_H_ */
