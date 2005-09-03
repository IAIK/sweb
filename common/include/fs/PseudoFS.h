#include "types.h"


class PseudoFS
{
public:
   
   static PseudoFS *getInstance();
   
   uint8 *getFilePtr(char *file_name);



private:

   PseudoFS();

   static PseudoFS *instance_;
   
};
