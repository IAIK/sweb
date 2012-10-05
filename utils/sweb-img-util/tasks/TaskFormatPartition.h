/**
 * Filename: TaskFormatPartition.h
 * Description:
 *
 * Created on: 05.09.2012
 * Author: chris
 */

#ifndef TASKFORMATPARTITION_H_
#define TASKFORMATPARTITION_H_

#include "UtilTask.h"

#include "fs/FsDefinitions.h"

/**
 * @class
 */
class TaskFormatPartition : public UtilTask
{
public:
  TaskFormatPartition(Program& image_util);

  virtual ~TaskFormatPartition();

  /**
   * executes the taks
   */
  virtual void execute(void);

  /**
   * returns the char identifying this option (e.g. h for help)
   */
  virtual char getOptionName(void) const;

  virtual const char* getDescription(void) const;

private:

  enum { ARG_FS_NAME = 4, ARG_ZONE_SIZE = 5, ARG_FS_VERSION = 6, ARG_OTHER_1 = 7 };

  /**
   * reads the zone / cluster size in bytes from the passed
   * arguments and returns it
   * @return input zone size
   */
  sector_addr_t getZoneSize(void);

  /**
   * reads the version to format from the passed arguments
   * NOTE: this parameter is optional and will default to 1 if not given
   * @return version to create
   */
  int32 getVersion(void);

  /**
   * formats minix on the given Device
   */
  void createMinixPartition(FsDevice* device);
};

#endif /* TASKFORMATPARTITION_H_ */
