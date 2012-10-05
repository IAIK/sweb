/**
 * @file FileDescriptor.h
 */

#ifndef FILEDESCRIPTOR_H__
#define FILEDESCRIPTOR_H__

#include "fs/FsDefinitions.h"
#include "types.h"

class File;
class Thread;

/**
 * @class FileDescriptor
 */
class FileDescriptor
{
  protected:
    /**
     * the file descriptor
     */
    fd_size_t fd_;

    /**
     * the file object
     */
    File* file_;

    /**
     * current cursor-position
     */
    file_size_t cursor_pos_;

    /**
     * the Thread owing the FD (used to avoid FD-Hijacking)
     * NOTE: potential locking-problem!!!
     */
    Thread* owner_;

    // append mode?
    bool append_mode_;

    // non blocking mode?
    bool nonblocking_mode_;

    // read mode?
    bool read_mode_;

    // write mode?
    bool write_mode_;

    // Synchronous mode
    bool synchronous_;

  public:
    /**
     * constructor
     * @param file the file to create the fd for
     * @param owner
     * @param append_mode
     * @param nonblocking_mode
     */
    FileDescriptor ( File* file, Thread* owner,
                     bool append_mode = false, bool nonblocking_mode = false );

    /**
     * destructor
     */
    virtual ~FileDescriptor();

    /**
     * get the file descriptor
     * @return the fd
     */
    fd_size_t getFd() { return fd_; }

    /**
     * get the file
     * @return the file
     */
    File* getFile() { return file_; }

    /**
     * getting the owing Thread of the FD
     * @return the owener of the FD
     */
    Thread* getOwner(void);

    /**
     * getting the current cursor position
     */
    file_size_t getCursorPos(void) const;

    /**
     * setting the position of the Cursor
     * @param new_pos sets a new Cursor position
     * @param movement moves the cursor n-bytes from the current position
     * @param to_the_end determines the relative move-direction, true moves the cursor
     * to the right/end; false to the beginning/left
     * @return the new current cursor position
     */
    file_size_t setCursorPos(file_size_t new_pos);
    file_size_t moveCursor(file_size_t movement, bool to_the_end = true);

    /**
     * is File opened in Append mode?
     * @return true if the file is opened in append mode; false if not
     */
    bool appendMode(void) const;

    /**
     * is the File opened in Nonblocking mode?
     * @return true if the file is opened in the non-blocking mode; false if blocking
     */
    bool nonblockingMode(void) const;

    /**
     * setting the state of the read-mode
     *
     * @param mode the new read-mode state
     */
    void setReadMode(bool mode);

    /**
     * setting the state of the write-mode
     *
     * @param mode the new write-mode state
     */
    void setWriteMode(bool mode);

    /**
     * is the File opened in read-mode?
     *
     * @return state of the read-mode
     */
    bool readMode(void) const;

    /**
     * is the File opened in read-mode?
     *
     * @return state of the read-mode
     */
    bool writeMode(void) const;

    /**
     * should file changes be written (immediately) to the file
     * by default the flag is set to false; NOTE: the flag has no effect
     * if the File resides on a FileSystem that is mounted with MS_SYNCHRONOUS
     *
     * @param mode
     */
    void setSynchroniousWrite(bool mode);

    /**
     * getting the current state of the synchronize mode
     *
     * @return
     */
    bool synchronizeMode(void) const;

    /**
     * add fd to global fd list
     * @param fd
     */
    static void add(FileDescriptor* fd);

    /**
     * remove fd from global fd list
     * @param fd
     */
    static void remove(FileDescriptor* fd);

    /**
     * remove fd from global fd list
     * @param fd
     */
    static bool remove(fd_size_t fd);

    /**
     * getting a FileDescriptor
     * @param
     * @return
     */
    static FileDescriptor* getFileDescriptor(fd_size_t fd);
};

#endif // FILEDESCRIPTOR_H_
