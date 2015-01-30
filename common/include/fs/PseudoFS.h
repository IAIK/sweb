/**
 * @file PseudoFS.h
 */

#ifndef PSEUDO_FS_H__
#define PSEUDO_FS_H__

#include "types.h"

/**
 * @class PseudoFS
 */
class PseudoFS
{
  public:

    typedef struct
    {
        char *file_name;
        char *file_start;
        size_t file_length;
    } FileIndexStruct;

    /**
     * Sigleton access method
     * @return the sigleton istance
     */
    static PseudoFS *getInstance();

    /**
     * returns the file pointer by the file name
     * @param file_name the file name
     * @return the file pointer
     */
    uint8 *getFilePtr(const char *file_name);

    /**
     * returns the files index
     * @param file_name the wanted file name
     * @return the file index
     */
    FileIndexStruct* getFileIndex(const char *file_name);

    /**
     * returns the number of files
     * @return the number of files
     */
    size_t getNumFiles() const;

    /**
     * returns the file by the given number
     * @param number the number looking for
     * @return the file
     */
    char* getFileNameByNumber(size_t number) const;

  private:

    /**
     * constructor
     */
    PseudoFS();

    static PseudoFS *instance_;

};

#endif
