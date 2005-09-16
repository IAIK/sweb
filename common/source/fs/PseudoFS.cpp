

#include "PseudoFS.h"
#include "ArchCommon.h"
#include "console/kprintf.h"

PseudoFS *PseudoFS::instance_=0;
   
PseudoFS *PseudoFS::getInstance()
{
   if (!instance_)
      instance_ = new PseudoFS();
   return instance_;
}

typedef struct 
{
  char  *file_name;
  char  *file_start;
  uint32 file_length;
}FileIndexStruct;

PseudoFS::PseudoFS()
{
   kprintfd("In pseudo fs\n");
   // first check if we have modules
   uint32 num_modules = ArchCommon::getNumModules();
   if (!num_modules)
   {
      kprintf("Error, no modules so I can't get a file ptr\n");
      return;
   }
   
   // module 0 is the one we are interested in
   
   uint32 start_address = ArchCommon::getModuleStartAddress(0);
   uint32 end_address = ArchCommon::getModuleEndAddress(0);
   
   kprintfd("Start Addr %x End Addr %x\n",start_address,end_address);
   
   uint32 *start = (uint32*)start_address;
   uint32 i;
   //uint32 magic = start[0];
   uint32 num_files = start[1];
   FileIndexStruct * files = (FileIndexStruct *)&start[2];
   
   for (i=0;i<num_files;++i)
   {
      files[i].file_name = (char *)((uint32)files[i].file_name + start_address);
      files[i].file_start = (char *)((uint32)files[i].file_start + start_address);
      kprintfd("Filename %s, filesize %d\n",files[i].file_name,files[i].file_length);
   }
   
}

uint8 *PseudoFS::getFilePtr(char *file_name)
{
   kprintfd("In pseudo fs\n");
   // first check if we have modules
   uint32 num_modules = ArchCommon::getNumModules();
   if (!num_modules)
   {
      kprintf("Error, no modules so I can't get a file ptr\n");
      return 0;
   }
   
   // module 0 is the one we are interested in
   
   uint32 start_address = ArchCommon::getModuleStartAddress(0);
   uint32 end_address = ArchCommon::getModuleEndAddress(0);
   
   kprintfd("Start Addr %x End Addr %x\n",start_address,end_address);
   
   uint32 *start = (uint32*)start_address;
   uint32 i;
   //uint32 magic = start[0];
   uint32 num_files = start[1];
   FileIndexStruct * files = (FileIndexStruct *)&start[2];
   
   for (i=0;i<num_files;++i)
   {
      char *a = file_name;
      char *b = files[i].file_name;
      uint32 c = 1;
      while (*a && *b)
      {
         if (*a != *b)
         {
            c = 0;
            break;
         }
         ++a,b++;
         
      }
      if (c)
         return (uint8*)files[i].file_start;
   }
   
   return 0;
   
}

//----------------------------------------------------------------------
uint32 PseudoFS::getNumFiles() const
{
  kprintfd("In pseudo fs\n");
  // first check if we have modules
  uint32 num_modules = ArchCommon::getNumModules();
  if (!num_modules)
  {
    kprintf("Error, no modules so I can't get a file ptr\n");
    return 0;
  }

  // module 0 is the one we are interested in

  uint32 start_address = ArchCommon::getModuleStartAddress(0);
  uint32 end_address = ArchCommon::getModuleEndAddress(0);

  kprintfd("Start Addr %x End Addr %x\n",start_address,end_address);

  uint32 *start = (uint32*)start_address;
  uint32 num_files = start[1];
  return num_files;
}

//----------------------------------------------------------------------
char* PseudoFS::getFileNameByNumber(uint32 number) const
{
  kprintfd("In pseudo fs\n");
  // first check if we have modules
  uint32 num_modules = ArchCommon::getNumModules();
  if (!num_modules)
  {
    kprintf("Error, no modules so I can't get a file ptr\n");
    return 0;
  }

  // module 0 is the one we are interested in

  uint32 start_address = ArchCommon::getModuleStartAddress(0);
  uint32 end_address = ArchCommon::getModuleEndAddress(0);

  kprintfd("Start Addr %x End Addr %x\n",start_address,end_address);

  uint32 *start = (uint32*)start_address;
  uint32 num_files = start[1];

  if (num_files < number)
  {
    kprintf("Error: no file with whis number in PseudoFS\n");
    return 0;
  }

  FileIndexStruct * files = (FileIndexStruct *)&start[2];
  return files[number].file_name;
}

