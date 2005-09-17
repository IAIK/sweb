
//
// CVS Log Info for $RCSfile: PseudoFS.h,v $
//
// $Id: PseudoFS.h,v 1.3 2005/09/17 09:33:55 davrieb Exp $
// $Log$
//
//

#ifndef PseudoF_h__
#define PseudoF_h__

#include "types.h"

class PseudoFS
{
public:

  typedef struct
  {
    char  *file_name;
    char  *file_start;
    uint32 file_length;
  }FileIndexStruct;

  static PseudoFS *getInstance();

  uint8 *getFilePtr(char *file_name);

  FileIndexStruct* getFileIndex(char *file_name);

  uint32 getNumFiles() const;

  char* getFileNameByNumber(uint32 number) const;

private:

  PseudoFS();

  static PseudoFS *instance_;

};

#endif
