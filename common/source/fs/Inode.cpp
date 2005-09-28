// Projectname: SWEB
// Simple operating system for educational purposes

#include "fs/Inode.h"
#include "assert.h"
#include "fs/File.h"

#define ERROR_FNE "Error: the file does not exist."
#define ERROR_FE  "Error: the file is exists."

//---------------------------------------------------------------------------
int32 Inode::insertOpenedFiles(File* file)
{
  if(file == 0)
  {
    // ERROR_FNE
    return -1;
  }
  
  if(i_files_.included(file) == true)
  {
    // ERROR_FE
    return -1;
  }
  i_files_.pushBack(file);
  
  return 0;
}

//---------------------------------------------------------------------------
int32 Inode::removeOpenedFiles(File* file)
{
  if(file == 0)
  {
    // ERROR_FNE
    return -1;
  }
  
  if(i_files_.included(file) == true)
    i_files_.remove(file);
  else
  {
    // ERROR_FNE
    return -1;
  }
  
  return 0;
}

