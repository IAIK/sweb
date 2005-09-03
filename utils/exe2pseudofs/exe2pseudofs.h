#ifndef _PSEUDOFS_H_
#define _PSEUDOFS_H_
//----------------------------------------------------------------------
//  $Id: exe2pseudofs.h,v 1.2 2005/09/03 13:01:54 btittelbach Exp $
//----------------------------------------------------------------------
//  $Log: exe2pseudofs.h,v $
//  Revision 1.1  2005/08/30 15:59:50  btittelbach
//  Exe2PseudoFS Proggy geschrieben, welches Datein in ein SuperSimple RO Filesystem schreibt,
//  welches dann per Grub in den Speicher geladen werden kann.
//
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


struct FileIndexStruct
{
  char  *file_name;
  char  *file_start;
  int file_length;
};


int const pseudofs_magic_number_ = 0x20DF97A1;
#endif
