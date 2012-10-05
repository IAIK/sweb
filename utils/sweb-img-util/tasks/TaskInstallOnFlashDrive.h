/**
 * Filename: TaskCopyExecutables.h
 * Description:
 *
 * Created on: 05.09.2012
 * Author: chris
 */

#ifndef TASK_INSTALL_ON_FLASH_DEVICE_H_
#define TASK_INSTALL_ON_FLASH_DEVICE_H_

#include "UtilTask.h"

class TaskInstallOnFlashDrive : public UtilTask
{
public:
  TaskInstallOnFlashDrive(Program& image_util);
  virtual ~TaskInstallOnFlashDrive();

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

  /**
   *
   * @return true to proceed; false quit!
   */
  bool printInitialWarning(void);

  /**
   *
   */
  bool checkFilesize(const std::string& device_name);

  /**
   * determines the size of the given device in bytes
   *
   * @param device e.g. "dev/sda1"
   * @return the size of the device in bytes
   */
  uint64_t getDeviceSize(const char* device);

  /**
   * displays a warning message and expects the user to enter
   * yes in order to continue
   *
   * @param message
   * @return true if the user wants to continue; false if not!
   */
  bool promtToProceed(const char* message);

  /**
   * makes a one to one copy of the loaded image file on the given device
   *
   * @param img_file the filename of the img-file
   * @param device the name of the device to copy the image file to
   * @return true / false
   */
  bool copyImageToDevice(const char* device);

};

#endif // TASK_INSTALL_ON_FLASH_DEVICE_H_
