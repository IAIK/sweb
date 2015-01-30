/**
 * @file PseudoFS.cpp
 */

#include "PseudoFS.h"
#include "ArchCommon.h"
#include "console/kprintf.h"
#include "console/debug.h"

PseudoFS *PseudoFS::instance_ = 0;

PseudoFS *PseudoFS::getInstance()
{
  if (!instance_)
    instance_ = new PseudoFS();
  return instance_;
}

PseudoFS::PseudoFS()
{
  // first check if we have modules
  size_t num_modules = ArchCommon::getNumModules();
  if (!num_modules)
  {
    kprintf("Error, no modules so I can't get a file ptr\n");
    return;
  }

  // module 0 is the one we are interested in

  size_t start_address = ArchCommon::getModuleStartAddress(0);
  size_t end_address = ArchCommon::getModuleEndAddress(0);

  debug(PSEUDOFS, "Start Addr %x End Addr %x\n", start_address, end_address);

  size_t *start = (size_t*) start_address;
  size_t i;
  //size_t magic = start[0];
  size_t num_files = start[1];
  FileIndexStruct * files = (FileIndexStruct *) &start[2];

  for (i = 0; i < num_files; ++i)
  {
    files[i].file_name = (char *) ((size_t) files[i].file_name + start_address);
    files[i].file_start = (char *) ((size_t) files[i].file_start + start_address);
    debug(PSEUDOFS, "Filename %s, filesize %d\n", files[i].file_name, files[i].file_length);
  }

}

uint8 *PseudoFS::getFilePtr(const char *file_name)
{
  // first check if we have modules
  size_t num_modules = ArchCommon::getNumModules();
  if (!num_modules)
  {
    kprintf("Error, no modules so I can't get a file ptr\n");
    return 0;
  }

  // module 0 is the one we are interested in

  size_t start_address = ArchCommon::getModuleStartAddress(0);
  size_t end_address = ArchCommon::getModuleEndAddress(0);

  debug(PSEUDOFS, "Start Addr %x End Addr %x\n", start_address, end_address);

  size_t *start = (size_t*) start_address;
  size_t i;
  //size_t magic = start[0];
  size_t num_files = start[1];
  FileIndexStruct * files = (FileIndexStruct *) &start[2];

  for (i = 0; i < num_files; ++i)
  {
    char const *a = file_name;
    char *b = files[i].file_name;
    while (*a && *b && (*a == *b))
      ++a, b++;

    if ((a - file_name) && (b - files[i].file_name) && !*a && !*b)
      return (uint8*) files[i].file_start;
  }

  return 0;

}

size_t PseudoFS::getNumFiles() const
{
  // first check if we have modules
  size_t num_modules = ArchCommon::getNumModules();
  if (!num_modules)
  {
    kprintf("Error, no modules so I can't get a file ptr\n");
    return 0;
  }

  // module 0 is the one we are interested in

  size_t start_address = ArchCommon::getModuleStartAddress(0);
  size_t end_address = ArchCommon::getModuleEndAddress(0);

  debug(PSEUDOFS, "Start Addr %x End Addr %x\n", start_address, end_address);

  size_t *start = (size_t*) start_address;
  size_t num_files = start[1];
  return num_files;
}

char* PseudoFS::getFileNameByNumber(size_t number) const
{
  // first check if we have modules
  size_t num_modules = ArchCommon::getNumModules();
  if (!num_modules)
  {
    kprintf("Error, no modules so I can't get a file ptr\n");
    return 0;
  }

  // module 0 is the one we are interested in

  size_t start_address = ArchCommon::getModuleStartAddress(0);
  size_t end_address = ArchCommon::getModuleEndAddress(0);

  debug(PSEUDOFS, "Start Addr %x End Addr %x\n", start_address, end_address);

  size_t *start = (size_t*) start_address;
  size_t num_files = start[1];

  if (num_files < number)
  {
    kprintf("Error: no file with whis number in PseudoFS\n");
    return 0;
  }

  FileIndexStruct * files = (FileIndexStruct *) &start[2];
  return files[number].file_name;
}

PseudoFS::FileIndexStruct* PseudoFS::getFileIndex(const char *file_name)
{
  // first check if we have modules
  size_t num_modules = ArchCommon::getNumModules();
  if (!num_modules)
  {
    kprintf("Error, no modules so I can't get a file ptr\n");
    return 0;
  }

  // module 0 is the one we are interested in

  size_t start_address = ArchCommon::getModuleStartAddress(0);
  size_t end_address = ArchCommon::getModuleEndAddress(0);

  debug(PSEUDOFS, "Start Addr %x End Addr %x\n", start_address, end_address);

  size_t *start = (size_t*) start_address;
  size_t i;
  //size_t magic = start[0];
  size_t num_files = start[1];
  FileIndexStruct * files = (FileIndexStruct *) &start[2];

  for (i = 0; i < num_files; ++i)
  {
    char const *a = file_name;
    char *b = files[i].file_name;
    size_t c = 1;
    while (*a && *b)
    {
      if (*a != *b)
      {
        c = 0;
        break;
      }
      ++a, b++;

    }
    if (c)
      return &(files[i]);
  }

  return 0;
}

