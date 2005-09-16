#include "types.h"


class PseudoFS
{
public:
   
   static PseudoFS *getInstance();
   
   uint8 *getFilePtr(char *file_name);

   uint32 getNumFiles() const;

   char* getFileNameByNumber(uint32 number) const;

private:

   PseudoFS();

   static PseudoFS *instance_;
   
};
