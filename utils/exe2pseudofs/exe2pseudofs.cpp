//----------------------------------------------------------------------
//  $Id: exe2pseudofs.cpp,v 1.2 2005/09/03 13:01:54 btittelbach Exp $
//----------------------------------------------------------------------
//  $Log: exe2pseudofs.cpp,v $
//  Revision 1.1  2005/08/30 15:59:50  btittelbach
//  Exe2PseudoFS Proggy geschrieben, welches Datein in ein SuperSimple RO Filesystem schreibt,
//  welches dann per Grub in den Speicher geladen werden kann.
//
//

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "exe2pseudofs.h"

//just unix for now
#define DIRECTORY_SEPERATOR '/'

int number_of_files_ = 0;
int index_offset_ = 2*sizeof(int);
int data_offset_ = 0;
FileIndexStruct *file_index_list_=0;
int image_fd_=-1;


char *base_name (char const *name)
{
  char const *base = name;
  char const *p;

  for (p = base; *p; p++)
  {
    if (*p == DIRECTORY_SEPERATOR)
    {
      do 
        p++;
      while (*p == DIRECTORY_SEPERATOR);

      if (! *p)
      {
        if (*base == DIRECTORY_SEPERATOR)
          base = p - 1;
        break;
      }
      base = p;
  }
}

  return (char *) base;
}

void writeFilename(int file_number, char* file_name)
{
  file_index_list_[file_number].file_name=reinterpret_cast<char*>(lseek(image_fd_,0,SEEK_CUR));
  write(image_fd_, file_name,strlen(file_name)+1);
}

void writeFileContents(int file_number, int fd)
{
  file_index_list_[file_number].file_start=reinterpret_cast<char*>(lseek(image_fd_,0,SEEK_CUR));
  int buffer_size=4096;
  char buffer[buffer_size];
  int bytes_read=0;
  int bytes_read_total=0;
  do
  {
    bytes_read=read(fd, &buffer, buffer_size);
    write(image_fd_, &buffer, bytes_read);
    bytes_read_total+=bytes_read;
  } while (bytes_read > 0);
  file_index_list_[file_number].file_length=bytes_read_total;
}

void writeFileIndex()
{
  lseek(image_fd_,index_offset_,SEEK_SET);
  write(image_fd_, file_index_list_, number_of_files_ * sizeof(FileIndexStruct));
}

int main (int argc, char *argv[])
{
  int file_fds[argc-2];
  unsigned int c=0;
  
  if (argc < 3)
  {
    printf("Syntax:\n%s <pseudofs image file> <file1> [file2 [file3 [....]]]\n\n",argv[0]);
    return -1;
  }
  
  image_fd_ = open(argv[1],  O_WRONLY| O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (image_fd_ < 0)
  {
    printf("Error opening %s\n\n",argv[1]);
    return -1;    
  }
  
  for (c=0; c< argc-2; ++c)
  {
    file_fds[c] = open(argv[c+2], O_RDONLY);
    if (file_fds[c] < 0)
      printf("Error opening %s, not including it in pseudo-fs\n",argv[c+2]);
    else
      number_of_files_++;
  }
  
  file_index_list_=new FileIndexStruct[number_of_files_];
  
  write(image_fd_, &pseudofs_magic_number_, sizeof(int));
  write(image_fd_, &number_of_files_, sizeof(int));
  
  index_offset_=lseek(image_fd_,0,SEEK_CUR);

  char null_char = 0;
  for (c=0; c < number_of_files_ * sizeof(FileIndexStruct); ++c)
    write(image_fd_, &null_char, 1);
  
  data_offset_=lseek(image_fd_,0,SEEK_CUR);
  
  for (unsigned int file=0; file < number_of_files_; ++file)
    if (file_fds[file] > -1)  
    {
      writeFilename(file,base_name(argv[file+2]));
      writeFileContents(file,file_fds[file]);
      close(file_fds[file]);
    }
  writeFileIndex();
  close(image_fd_);
  delete[] file_index_list_;
  printf("Successfully created %s with %d Files\n",argv[1],number_of_files_);
}
