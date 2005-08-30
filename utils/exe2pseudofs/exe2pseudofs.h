//----------------------------------------------------------------------
//  $Id: exe2pseudofs.h,v 1.1 2005/08/30 15:59:50 btittelbach Exp $
//----------------------------------------------------------------------
//  $Log: exe2pseudofs.h,v $
//

//~ PseudoFS:
//~ -PseudoFS Header
//~ -PseudoFS File Index List
//~ -Data

//~ PseudoFS Header:
//~ type		field
//~ -int		MagicNumber
//~ -int		number of files

//~ Pseudo FS File Index List:
//~ type		field
//~ -char*		 to FileName in Pseudofs
//~ -char*		char* to Start of Object File Image
//~ -int		length of Object File Image

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

struct FileIndexStruct
{
  char  *file_name;
  char  *file_start;
  int file_length;
};


int const magic_number_ = 0x20DF97A1;

int number_of_files_ = 0;
int index_offset_ = 2*sizeof(int);
int data_offset_ = 0;
FileIndexStruct *file_index_list_=0;
int image_fd_=-1;
