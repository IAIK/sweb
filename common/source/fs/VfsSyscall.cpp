/**
 * @file VfsSyscall.cpp
 */

#include "fs/VfsSyscall.h"

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
#include "util/string.h"
#include "assert.h"
#include "mm/kmalloc.h"
#else
#include <cstring>
#include <assert.h>
#endif

#include "fs/FsErrorCodes.h"

#include "fs/inodes/Inode.h"
#include "fs/inodes/Directory.h"
#include "fs/inodes/File.h"

#include "fs/FsWorkingDirectory.h"
#include "fs/FsSmartLock.h"
#include "fs/FileDescriptor.h"
#include "fs/Dirent.h"
#include "fs/DIR.h"
#include "fs/Statfs.h"

#include "fs/device/FsDevice.h"
#include "fs/device/FsDeviceVirtual.h"

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
#include "arch_bd_virtual_device.h"
#include "arch_bd_manager.h"
#include "console/kprintf.h"
#else
#include "debug_print.h"
#endif

char VfsSyscall::LAST_DEFINED_PATH_SEPARATOR = '/';

//
// the following methods are only available under SWEB and NOT in the
// guest OS
//
#ifndef USE_FILE_SYSTEM_ON_GUEST_OS

#define ROOT_PARTITION      "idea1"

// Singleton static instance
VfsSyscall* VfsSyscall::instance_ = NULL;

void VfsSyscall::createVfsSyscall(void)
{
  if(instance_)
    return;

  instance_ = new VfsSyscall();
}

VfsSyscall* VfsSyscall::instance()
{
  return instance_;
}

VfsSyscall::VfsSyscall(char path_separator) : PATH_SEPARATOR(path_separator),
     mounted_fs_(), mount_lock_("vfs_mounts_lock"), root_(NULL), fs_pool_()
{
  LAST_DEFINED_PATH_SEPARATOR = path_separator;
  debug(VFSSYSCALL, "starting to create VfsSyscall Singleton-instance\n");
  assert(initRootFs() == 0);
  debug(VFSSYSCALL, "VfsSyscall was successfully init\n");
}

VfsSyscall::~VfsSyscall()
{
}

int32 VfsSyscall::initRootFs(void)
{
  // Initializing the root-file system
  debug(VFSSYSCALL, "init root filesystem\n");

  // getting the partition where the root is stored
  debug(VFSSYSCALL, "get BDVirtualDevice object of root-device\n");
  BDVirtualDevice* bddev = BDManager::getInstance()->getDeviceByName(ROOT_PARTITION);

  // assert the root-device exists
  assert(bddev != NULL);

  FsDevice* root_fs_dev = new FsDeviceVirtual(bddev);
  debug(VFSSYSCALL, "root device \"%s\" no=%d\n", bddev->getName(), bddev->getDeviceNumber());
  debug(VFSSYSCALL, "root file system partition identifier=%x\n", bddev->getPartitionType());

  // create the root-file system
  root_ = fs_pool_.getNewFsInstance(root_fs_dev, bddev->getPartitionType(), /*MS_SYNCHRONOUS |*/ MS_NOATIME);

  // failed to create the root-file system
  if(root_ == NULL)
  {
    debug(VFSSYSCALL, "mount - ERROR failed to instantiate new file-system\n");
    return -1; //
  }

  mounted_fs_.push_back(root_);

  debug(VFSSYSCALL, "Root FileSystem is \"%s\"\n", root_->getName());
  return 0;
}

void VfsSyscall::unmountRoot(void)
{
  mount_lock_.acquire("VfsSyscall::unmountRoot");

  // unmount all mounted FileSystems in the reverse order of mounting
  // (last mounted - first removed)
  if(mounted_fs_.size() > 1)
  {
    for(uint32 i = mounted_fs_.size()-1; i > 0; i--)
    {
      // TODO unmount FileSystem properly
    }
  }

  delete root_;
  root_ = NULL;
  mount_lock_.release("VfsSyscall::unmountRoot");
}

#else

VfsSyscall::VfsSyscall(FsDevice* fs_device, uint8 partition_type,
    char path_separator) : PATH_SEPARATOR(path_separator), root_(NULL), fs_pool_()
{
  // create root-fs instance
  root_ = fs_pool_.getNewFsInstance(fs_device, partition_type, MS_NOATIME);
  assert(root_ != NULL);

  LAST_DEFINED_PATH_SEPARATOR = path_separator;
}

VfsSyscall::~VfsSyscall()
{
  if(root_ != NULL)
  {
    delete root_;
  }
}

#endif

bool VfsSyscall::resolveAndWriteLockParentDirectory(FsWorkingDirectory* wd_info,
    const char* pathname, FailCondition fail_if,
    Directory*& parent, char*& last_part, bool& file_extists)
{
  // 1. splitting given pathname into path and filename
  char* path = NULL;
  char* filename = NULL;
  assert(splitPath(pathname, &path, &filename));

  // 2. check length of paths
  if(strlen(path) >= PATH_MAX || strlen(filename) >= NAME_MAX)
  {
    debug(VFSSYSCALL, "resolveParentDirectory() - ERROR - path is too long, sorry...!\n");

    delete[] path;
    delete[] filename;
    return false;
  }

  // 3. resolve parent-directory of the File
  Directory* parent_dir = resolveDirectory(wd_info, path);

  // from now on the path is no longer needed
  delete[] path;
  path = NULL;

  // resolved path does not point to a Directory, error
  if(parent_dir == NULL)
  {
    debug(VFSSYSCALL, "resolveParentDirectory() - ERROR - failed to resolve parent directory of \"%s\"\n", pathname);
    delete[] filename;
    return false;
  }

  // the FileSystem that will hold the new file
  FileSystem* fs = parent_dir->getFileSystem();

  // 4. establish mutual exclusion to the data of the parent Directory
  parent_dir->getLock()->acquireWriteBlocking();

  // 5. check the existence of the child-inode
  if(parent_dir->isChild(filename))
  {
    file_extists = true;
  }
  else
  {
    file_extists = false;
  }

  // method should fail if child exists / does not exist
  if( (fail_if == FAIL_IF_CHILD_EXISTS && file_extists) ||
      (fail_if == FAIL_IF_CHILD_DOES_NOT_EXIST && !file_extists) )
  {
    parent_dir->getLock()->releaseWrite();
    fs->releaseInode( parent_dir );
    delete[] filename;

    return false;
  }

  // 6. check permission; is the user allowed to create a file here?
  if( !isOperationPermitted(parent_dir, WRITE) )
  {
    // WRITE-access to parent is not permitted for wd_info!
    parent_dir->getLock()->releaseWrite();
    fs->releaseInode( parent_dir );

    delete[] filename;
    return false;
  }

  // set output-parameters and return:
  parent = parent_dir;
  last_part = filename;

  return true;
}

int32 VfsSyscall::mkdir(FsWorkingDirectory* wd_info, const char* pathname, mode_t mode)
{
  if(pathname == NULL)
  {
    return FS_ERROR_INVALID_ARGUMENT;
  }

  debug(VFSSYSCALL, "mkdir - create new empty dir \"%s\"\n", pathname);

  Directory* parent;
  char* folder_name = NULL;
  bool inode_already_exists;

  // resolve parent Directory of folder to create new; method call includes
  // check if the there is already an inode in the parent-directory
  if(!resolveAndWriteLockParentDirectory(wd_info, pathname,
      FAIL_IF_CHILD_EXISTS, parent, folder_name, inode_already_exists))
  {
    debug(VFSSYSCALL, "mkdir - ERROR failed to resolve/lock/write-access the parent Directory!\n");
    return -1;
  }

  // the FileSystem of the parent-Directory
  FileSystem* fs = parent->getFileSystem();

  // creation time
  unix_time_stamp current_time = getCurrentTimeStamp();
  inode_id_t new_dir_id = 0;

  // delegate mkdir() call to the file-system in charge
  int32 ret_val = fs->mkdir(parent, folder_name, current_time, mode, 0, 0, new_dir_id);

  if(ret_val == 0)
  {
    parent->addChild(folder_name, new_dir_id);
    parent->incrReferenceCount();

    parent->updateCTime(current_time);
    parent->updateModTime(current_time);

    // rewrite parent I-Node on FS
    rewriteParentDirectory(fs, parent);
  }

  // release borrowed parent i-node
  parent->getLock()->releaseWrite();
  fs->releaseInode(parent);

  delete[] folder_name;

  debug(VFSSYSCALL, "mkdir - finished with return value=%d\n", ret_val);
  return ret_val;
}

int32 VfsSyscall::rmdir ( FsWorkingDirectory* wd_info, const char* pathname )
{
  if(pathname == NULL)
  {
    return FS_ERROR_INVALID_ARGUMENT; // ENOENT
  }

  debug(VFSSYSCALL, "rmdir - going to remove \"%s\"\n", pathname);

  // resolve the path of the Directory that should be removed
  Directory* dir_to_rm = resolveDirectory(wd_info, pathname);

  if(dir_to_rm == NULL)
  {
    debug(VFSSYSCALL, "rmdir - failed to resolve the directory.\n");
    return FS_ERROR_FAILED_TO_RESOLVE_PATH; // ENOTDIR
  }

  // the FileSystem where the new Directory is created on
  FileSystem* fs = dir_to_rm->getFileSystem();

  Directory* parent;
  char* folder_name = NULL;
  bool inode_already_exists;

  // resolve parent Directory of folder to create new; method call includes
  // check if the there is already an inode in the parent-directory
  if(!resolveAndWriteLockParentDirectory(wd_info, pathname,
      FAIL_IF_CHILD_DOES_NOT_EXIST, parent, folder_name, inode_already_exists))
  {
    fs->releaseInode( dir_to_rm );

    debug(VFSSYSCALL, "rmdir - ERROR failed to resolve/lock/write-access the parent Directory!\n");
    return -1;
  }

  // lock file for writing
  dir_to_rm->getLock()->acquireWriteBlocking();

  // either the directory has no children or just two and these two are
  // the self-reference and the parent reference
  if(!dir_to_rm->isEmpty())
  {
    delete[] folder_name;

    // error: Directory is not empty!
    dir_to_rm->getLock()->releaseWrite();
    fs->releaseInode(dir_to_rm);

    parent->getLock()->releaseWrite();
    parent->getFileSystem()->releaseInode(parent);

    debug(VFSSYSCALL, "rmdir - ERROR directory to remove is NOT empty!\n");
    return FS_ERROR_DIR_NOT_EMPTY; // ENOTEMPTY
  }

  // delegate rmdir() call to the file-system in charge
  int32 ret_val = fs->rmdir(parent, dir_to_rm->getID(), folder_name);

  // from now on there is no guarantee that the Directory to remove
  // exists any longer:
  dir_to_rm->getLock()->releaseWrite();
  fs->releaseInode(dir_to_rm);

  if(ret_val == 0)
  {
    parent->removeChild(folder_name);
    parent->decrReferenceCount(); // one reference less

    unix_time_stamp current_time = getCurrentTimeStamp();

    parent->updateCTime(current_time);
    parent->updateModTime(current_time);

    // rewrite-parent:
    rewriteParentDirectory(parent->getFileSystem(), parent);
  }

  delete[] folder_name;

  // release Parent's Write lock and I-node cache reference
  parent->getLock()->releaseWrite();
  parent->getFileSystem()->releaseInode(parent);

  debug(VFSSYSCALL, "rmdir - finished with return value=%d\n", ret_val);
  return ret_val;
}

int32 VfsSyscall::link(FsWorkingDirectory* wd_info, const char* oldpath, const char* newpath)
{
  if(oldpath == NULL || newpath == NULL)
    return -1; // ENOENT

  debug(VFSSYSCALL, "link - CALL link to \"%s\" from \"%s\"\n", oldpath, newpath);

  // resolve file to be linked
  File* file_to_link = resolveFile(wd_info, oldpath);

  // just links to Files allowed!
  if(file_to_link == NULL)
  {
    debug(VFSSYSCALL, "link - ERROR failed to resolve \"%s\"!\n", oldpath);
    return -1; // EPERM
  }

  Directory* parent;
  char* filename = NULL;
  bool inode_already_exists;

  // resolve linked File's new parent Directory; resolving FAILS if there
  // is already an inode named like filename
  if(!resolveAndWriteLockParentDirectory(wd_info, newpath, FAIL_IF_CHILD_EXISTS, parent, filename, inode_already_exists))
  {
    file_to_link->getFileSystem()->releaseInode( file_to_link );

    debug(VFSSYSCALL, "link - ERROR failed to resolve/lock/write-access the parent Directory!\n");
    return -1;
  }

  // ERROR: file and link are not on the same file-system
  if(file_to_link->getFileSystem() != parent->getFileSystem())
  {
    parent->getLock()->releaseWrite();
    parent->getFileSystem()->releaseInode(parent);
    file_to_link->getFileSystem()->releaseInode(file_to_link);

    debug(VFSSYSCALL, "link - ERROR parent-directory of new path does not reside on same FS as existing file!\n");

    delete[] filename;
    return -1; // EXDEV
  }

  // establish a new link from the file to the given Directory
  FileSystem* fs = file_to_link->getFileSystem();

  // mutual exclusion for the parent-directory
  file_to_link->getLock()->acquireWriteBlocking();

  // delegate call to the FS in charge
  int32 ret_val = fs->link(file_to_link, parent, filename);

  if(ret_val == 0)
  {
    // adding new child to parent
    parent->addChild(filename, file_to_link->getID());

    unix_time_stamp current_time = getCurrentTimeStamp();

    parent->updateCTime(current_time);
    parent->updateModTime(current_time);
    file_to_link->updateCTime(current_time);

    // rewrite parent-directory
    rewriteParentDirectory(fs, parent);

    debug(VFSSYSCALL, "link - OK link() was successful!\n");
  }

  // release i-nodes
  parent->getLock()->releaseWrite();
  file_to_link->getLock()->releaseWrite();

  fs->releaseInode(file_to_link);
  fs->releaseInode(parent);

  delete[] filename;

  debug(VFSSYSCALL, "link - DONE with code (%d)\n", ret_val);
  return ret_val;
}

int32 VfsSyscall::unlink(FsWorkingDirectory* wd_info, const char *pathname)
{
  if(pathname == NULL)
    return -1; // ENOENT

  debug(VFSSYSCALL, "unlink - CALL removing \"%s\"\n", pathname);

  Directory* parent;
  char* filename = NULL;
  bool inode_already_exists;

  // resolve parent Directory of folder to create new; method call includes
  // check if the there is already an inode in the parent-directory
  if(!resolveAndWriteLockParentDirectory(wd_info, pathname,
      FAIL_IF_CHILD_DOES_NOT_EXIST, parent, filename, inode_already_exists))
  {
    debug(VFSSYSCALL, "unlink - ERROR failed to resolve/lock/write-access the parent Directory!\n");
    return -1;
  }

  // the FileSystem of the link's parent Directory
  FileSystem* fs = parent->getFileSystem();

  // get the File-object to unlink
  // WARNING: do NOT use resolveFile here, since the parent directory is
  // exclusively locked and can not be accessed! (DEADLOCK!)
  Inode* inode = parent->getInode(filename);

  // just links to Files allowed!
  if(inode == NULL || !(inode->getType() & Inode::InodeTypeFile))
  {
    debug(VFSSYSCALL, "unlink - ERROR file not found.\n");

    parent->getLock()->releaseWrite();
    fs->releaseInode(parent);

    delete[] filename;
    return -1; // EPERM
  }

  File* file_to_unlink = static_cast<File*>(inode);

  // lock file to unlink
  file_to_unlink->getLock()->acquireWriteBlocking();

  // delegate unlink call
  int32 ret_val = fs->unlink(file_to_unlink, parent, filename);

  if(ret_val == 0)
  {
    // update time fields:
    unix_time_stamp current_time = getCurrentTimeStamp();

    parent->updateCTime(current_time);
    parent->updateModTime(current_time);
    file_to_unlink->updateCTime(current_time);

    // adding new child to parent
    parent->removeChild(filename);

    // rewrite parent-directory
    rewriteParentDirectory(fs, parent);
    //fs->writeInode(parent);
  }

  parent->getLock()->releaseWrite();
  file_to_unlink->getLock()->releaseWrite();

  // release i-nodes
  fs->releaseInode(file_to_unlink);
  fs->releaseInode(parent);

  delete[] filename;

  debug(VFSSYSCALL, "unlink - DONE with code (%d)\n", ret_val);
  return ret_val;
}

DIR* VfsSyscall::opendir(FsWorkingDirectory* wd_info, const char* name)
{
  debug(VFSSYSCALL, "opendir - call; path=\"%s\"\n", name);

  // resolve given path
  Directory* dir = resolveDirectory(wd_info, name);

  if(dir == NULL)
  {
    debug(VFSSYSCALL, "opendir - invalid arguments\n");
    return NULL;
  }

  // just opening / loading the Directory is not enough, we actually
  // have to load the Directories children by calling the lookup()
  // method wich will load the unloaded children into memory
  // loading the Direcory's self reference
  //dir->getFileSystem()->lookup(dir, ".");

  debug(VFSSYSCALL, "opendir - OK\n");
  return new DIR(dir);
}

Dirent* VfsSyscall::readdir ( FsWorkingDirectory* wd_info __attribute__((unused)), DIR* dirp )
{
  debug(VFSSYSCALL, "readdir - call\n");

  // assert mutual exclusion:
  FsWriteSmartLock write_auto_lock( dirp->dir_->getLock() );

  Inode* child = dirp->dir_->getInode( dirp->cursor_pos );
  if(child == NULL)
  {
    debug(VFSSYSCALL, "readdir - end of directory\n");
    return NULL; // end of directory reached
  }

  // gathering informations about the current node
  Dirent* dirent = new Dirent(child->getID(), child->getName());

  if(dirent != NULL && dirp->dir_->doUpdateAccessTime())
  {
    // update the time of last access
    dirp->dir_->updateAccessTime( getCurrentTimeStamp() );
  }

  // increment cursor pos
  dirp->cursor_pos++;

  // release lock and cache-reference
  child->getFileSystem()->releaseInode(child);

  debug(VFSSYSCALL, "readdir - OK\n");
  return dirent;
}

void VfsSyscall::rewinddir(FsWorkingDirectory* wd_info __attribute__((unused)), DIR* dirp)
{
  debug(VFSSYSCALL, "rewinddir - call\n");

  if(dirp == NULL)
  {
    debug(VFSSYSCALL, "rewinddir - invalid arguments\n");
    return;
  }

  dirp->cursor_pos = 0;
  debug(VFSSYSCALL, "rewinddir - OK\n");
}

bool VfsSyscall::closedir(FsWorkingDirectory* wd_info __attribute__((unused)), DIR* dirp)
{
  debug(VFSSYSCALL, "closedir - call\n");

  if(dirp == NULL)
    return false;

  // delete DIR object (destructor releases acquired Directory-node)
  delete dirp;
  debug(VFSSYSCALL, "closedir - OK\n");
  return true;
}

int32 VfsSyscall::chdir ( FsWorkingDirectory* wd_info, const char* pathname )
{
  debug(VFSSYSCALL, "chdir - CALL\n");

  if(wd_info == NULL)
    return -1; // no thread, no wd-change!

  debug(VFSSYSCALL, "chdir - changing current directory to \"%s\"\n", pathname);
  return wd_info->setWorkingDir(pathname);
}

const char* VfsSyscall::getwd(FsWorkingDirectory* wd_info) const
{
  if(wd_info == NULL)
    return NULL;

  return wd_info->getWorkingDirPath();
}

int32 VfsSyscall::creat(FsWorkingDirectory* wd_info, const char *path, mode_t mode)
{
  // see http://pubs.opengroup.org/onlinepubs/009604599/functions/creat.html
  return this->open(wd_info, path, O_WRONLY|O_CREAT|O_TRUNC, mode);
}

int32 VfsSyscall::open(FsWorkingDirectory* wd_info, const char* pathname, int32 flag, mode_t mode)
{
  if(pathname == NULL)
    return -1;

  debug(VFSSYSCALL, "open() - opening \"%s\" with flags=%d\n", pathname, flag);

  // resolve path into an File-I-node
  File* file = resolveFile(wd_info, pathname);

  // the file does not exist ...
  if(file == NULL)
  {
    // ... but O_CREAT was specified, so create a new file
    if( (flag & O_CREAT) )
    {
      debug(VFSSYSCALL, "open() - file does not exist - O_CREAT is set - so create it:\n");

      // file does not exist, create it!
      file = createFile(wd_info, pathname, mode);
    }

    // ... error file does not exist, failed to open
    if( file == NULL )
    {
      debug(VFSSYSCALL, "open() - file does not exist - failed to open!\n");
      return -1; // ENOENT
    }
  }
  else if((flag & O_CREAT) && (flag & O_EXCL))
  {
    // file exists and O_EXCL and O_CREAT are specified:
    debug(VFSSYSCALL, "open() - FAIL - file exists + O_CREAT and O_EXCL are specified!\n");
    file->getFileSystem()->releaseInode(file);
    return -1; // EEXIST
  }

  debug(VFSSYSCALL, "open() - OK - file resolved!\n");

  // create a FileDescriptor for the File
  FileDescriptor* fd = createFDForFile(file, flag);

  if(fd == NULL)
  {
    // NOTE: if the creation of the FD failed the createFDForFile function
    // automatically releases the acquired File (Inode)
    debug(VFSSYSCALL, "open - ERROR not permitted to open file!\n");
    return -1;
  }

  FileDescriptor::add(fd);
  debug(VFSSYSCALL, "open() - FD nr is=%d!\n", fd->getFd());

  return fd->getFd();
}

FileDescriptor* VfsSyscall::createFDForFile(File* file, uint32 flag)
{
  // create a FileDescriptor object for the given file with the given flags
  bool append = (flag & O_APPEND) ? true : false;
  bool nonblock = (flag & O_NONBLOCK) ? true : false;

  // create a FD with the File and return it's id
  FileDescriptor* fd = new FileDescriptor(file, append, nonblock);

  if(fd == NULL)
  {
    // release File
    FileSystem* fs = file->getFileSystem();
    fs->releaseInode( file );

    return NULL;
  }

  if((flag & O_RDONLY) || (flag & O_RDWR))
  {
    fd->setReadMode(true);
  }

  if((flag & O_WRONLY) || (flag & O_RDWR))
  {
    fd->setWriteMode(true);
  }

  if((flag & O_SYNC))
  {
    fd->setSynchroniousWrite(true);
  }

  // finally before adding the fd - check the permissions of the user
  if( (fd->writeMode() && !isOperationPermitted(file, WRITE)) ||
      (fd->readMode() && !isOperationPermitted(file, READ)))
  {
    delete fd; // includes the release of the File

    debug(VFSSYSCALL, "createFDForFile() - FAIL - user is not permitted to open file!\n");
    return NULL;
  }

  // if truncated is specified and the File will be opened in write-mode
  // the contents of the File should be discarded
  if((flag & O_TRUNC) && fd->writeMode())
  {
    file->truncateProtected();
  }

  return fd;
}

File* VfsSyscall::createFile(FsWorkingDirectory* wd_info, const char* pathname, mode_t mode)
{
  debug(VFSSYSCALL, "createFile() - INFO - create new file %s\n", pathname);

  Directory* parent;
  char* filename = NULL;
  bool inode_already_exists;

  // resolve parent Directory of folder to create new
  if(!resolveAndWriteLockParentDirectory(wd_info, pathname, DO_NOT_FAIL, parent, filename, inode_already_exists))
  {
    debug(VFSSYSCALL, "createFile() - ERROR failed to resolve/lock/write-access the parent Directory!\n");
    return NULL;
  }

  FileSystem* fs = parent->getFileSystem();

  // check is the file already exists (or was created meanwhile)!
  if(inode_already_exists)
  {
    debug(VFSSYSCALL, "createFile() - INFO - parent already has a child named \"%s\"\n", filename);

    // file already exists, no need to re-create it
    // just resolve the File-object and return it
    Inode* inode = parent->getInode(filename);

    // release lock and parent-reference
    parent->getLock()->releaseWrite();
    fs->releaseInode( parent );
    delete[] filename;

    if(inode->getType() & Inode::InodeTypeFile)
    {
      debug(VFSSYSCALL, "createFile() - OK - file was created in the meantime, so return it!\n");
      return static_cast<File*>(inode);
    }
    // parent-dir has already a child named like the one to be created, but
    // the child is actually not a file -> error
    else
    {
      // release inode
      inode->getFileSystem()->releaseInode( inode );
      debug(VFSSYSCALL, "createFile() - ERROR - there exists already an object with the given name!\n");
      return NULL;
    }
  }

  // create new file in resolved parent directory
  File* new_file = fs->creat(parent, filename, mode, 0, 0);

  // file creation was successful
  if(new_file != NULL)
  {
    // add a new child to the parent-directory
    parent->addChild(filename, new_file->getID());

    // current time-stamp
    unix_time_stamp current_time = getCurrentTimeStamp();

    parent->updateCTime(current_time);
    parent->updateModTime(current_time);

    // rewrite changed parent I-Node
    rewriteParentDirectory(fs, parent);
  }

  // release Parent-Directory node
  parent->getLock()->releaseWrite();
  fs->releaseInode(parent);

  debug(VFSSYSCALL, "createFile() - OK - new file \"%s\" was successfully created\n", filename);

  delete[] filename;

  return new_file;
}

int32 VfsSyscall::close(FsWorkingDirectory* wd_info __attribute__((unused)), uint32 fd)
{
  debug(VFSSYSCALL, "close() - INFO - closing FD\n");

  if(!FileDescriptor::remove(fd))
  {
    return -1; // EBADF
  }

  // closing the file-descriptor
  return 0;
}

int32 VfsSyscall::read ( FsWorkingDirectory* wd_info __attribute__((unused)), fd_size_t fd, char* buffer, size_t count )
{
  debug(VFSSYSCALL, "read() - call\n");

  // translating the integer into a FileDescriptor object:
  FileDescriptor* fd_object = FileDescriptor::getFileDescriptor(fd);

  if(fd_object == NULL)
  {
    debug(VFSSYSCALL, "read() - invalid FD\n");
    return -1; //
  }

  File* file = fd_object->getFile();
  assert(file != NULL);

  if(!fd_object->readMode())
  {
    debug(VFSSYSCALL, "read() - no read-rights on the file.\n");
    return -1;
  }

  // read from file and return
  return file->read(fd_object, buffer, count);
}

int32 VfsSyscall::write ( FsWorkingDirectory* wd_info __attribute__((unused)), fd_size_t fd, const char *buffer, size_t count )
{
  debug(VFSSYSCALL, "write - CALL writing %d bytes to fd=%d\n", count, fd);

  // translating the integer into a FileDescriptor object:
  FileDescriptor* fd_object = FileDescriptor::getFileDescriptor(fd);

  if(fd_object == NULL)
  {
    debug(VFSSYSCALL, "write() - invalid FD\n");
    return -1; //
  }

  File* file = fd_object->getFile();

  if(!fd_object->writeMode())
  {
    debug(VFSSYSCALL, "write() - no write-rights on the file.\n");
    return -1;
  }

  // write to file and return
  int32 bytes_written = file->write(fd_object, buffer, count);

  // if the FileSystem does not use a write-trough cache and the write
  // operation was successful (writing at least one byte) AND the open()
  // call has the O_SYNC flag specified, then sync the File with the FileSystem
  if(!(file->getFileSystem()->getMountFlags() & MS_SYNCHRONOUS)
      && fd_object->synchronizeMode() && bytes_written > 0)
  {
    file->getFileSystem()->fsync(file);
  }

  return bytes_written;
}

l_off_t VfsSyscall::lseek ( FsWorkingDirectory* wd_info __attribute__((unused)), fd_size_t fd, l_off_t offset, uint8 whence )
{
  debug(VFSSYSCALL, "lseek() - seeking fd(%d) by=%d whence=%d\n", fd, offset, whence);

  // translating the integer into a FileDescriptor object:
  FileDescriptor* fd_object = FileDescriptor::getFileDescriptor(fd);

  if(fd_object == NULL)
  {
    debug(VFSSYSCALL, "lseek() - invalid fd\n");
    return -1; // EBADF
  }

  l_off_t new_offset = 0;

  if(whence == SEEK_SET)
  {
    // set file-cursor to offset
    new_offset = fd_object->setCursorPos(offset);
    debug(VFSSYSCALL, "lseek() - SEEK_SET by (%d); new_offset=%X\n", offset, new_offset);
  }
  else if(whence == SEEK_CUR)
  {
    // add offset to file cursors current position
    new_offset = fd_object->setCursorPos( fd_object->getCursorPos() + offset);
    debug(VFSSYSCALL, "lseek() - SEEK_CUR by (%d); new_offset=%X\n", offset, new_offset);
  }
  else if(whence == SEEK_END)
  {
    // file-cursor should be set to the filesize + offset
    new_offset = fd_object->setCursorPos(fd_object->getFile()->getFileSize() + offset);
    debug(VFSSYSCALL, "lseek() - SEEK_END by (%d); new_offset=%X\n", offset, new_offset);
  }
  else
  {
    new_offset = (l_off_t)-1; // error unknown whence EINVAL
    debug(VFSSYSCALL, "lseek() - ERROR unknown whence - lseek has no effect.\n");
  }

  return new_offset;
}

void VfsSyscall::sync(FsWorkingDirectory* wd_info __attribute__((unused)))
{
#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
  // Synchronize all mounted file systems
  mount_lock_.acquire("sync()");

  // TODO sync Root!
  for(uint32 i = 0; i < mounted_fs_.size(); i++)
  {
    mounted_fs_[i]->sync();
  }

  mount_lock_.release("sync()");
#else
  root_->sync();
#endif
}

int32 VfsSyscall::fsync(FsWorkingDirectory* wd_info __attribute__((unused)), int32 fd)
{
  debug(VFSSYSCALL, "fsync() - CALL\n");

  // resolve int-fd into object
  FileDescriptor* fd_object = FileDescriptor::getFileDescriptor(fd);

  if(fd_object == NULL)
  {
    debug(VFSSYSCALL, "fsync() - invalid FD\n");
    return -1; //
  }

  File* file = fd_object->getFile();
  FileSystem* fs = file->getFileSystem();

  // sync the file
  return fs->fsync(file);
}

statfs_s* VfsSyscall::statfs(FsWorkingDirectory* wd_info, const char *path)
{
  debug(VFSSYSCALL, "statfs() - CALL\n");

  // resolve path to an I-Node
  Inode* inode = resolvePath(wd_info, path);

  if(inode == NULL)
  {
    // no stats under this path
    debug(VFSSYSCALL, "statfs() - ERROR path seems to be invalid.\n");
    return NULL;
  }

  FileSystem* fs = inode->getFileSystem();

  statfs_s* stats = fs->statfs();
  fs->releaseInode(inode);

  debug(VFSSYSCALL, "statfs() - DONE\n");
  return stats;
}

uint32 VfsSyscall::getLastSeparatorPos(const char* full_path) const
{
  uint32 len = 0;
  while(full_path[len++] != '\0');

  for(uint32 i = len-1; i > 0; i--)
  {
    if(full_path[i] == PATH_SEPARATOR)
      return i;
  }

  // no separator in the string!
  return 0;
}

char* VfsSyscall::extractsPath(const char* full_path) const
{
  if(full_path == NULL)
    return NULL;

  // getting the position of the last path-separator in the string
  uint32 sep = getLastSeparatorPos(full_path);

  // special case: sep==0 can mean no separator at all or a direct root-child
  if(sep == 0)
  {
    //
    if(strlen(full_path) >= 1 && full_path[0] == PATH_SEPARATOR)
    {
      char* path = new char[2];
      path[0] = PATH_SEPARATOR;
      path[1] = '\0';

      return path;
    }
    // relative path without any separator
    else
    {
      return strdup("");
    }
  }

  // default case

  // extracting the path
  uint32 path_len = sep;
  char* path = new char[path_len+1];
  memcpy(path, full_path, path_len);
  path[path_len] = '\0';

  return path;
}

char* VfsSyscall::extractsPart(const char* full_path) const
{
  if(full_path == NULL)
    return NULL;

  uint32 sep = getLastSeparatorPos(full_path);
  if(sep != 0 || (sep == 0 && strlen(full_path) >= 1 && full_path[0] == PATH_SEPARATOR)) sep++;

  // extracting the last-part from the full_path
  uint32 part_len = strlen(full_path) - sep;
  char* part = new char[part_len+1];
  memcpy(part, full_path+sep, part_len);
  part[part_len] = '\0';

  return part;
}

bool VfsSyscall::splitPath(const char* full_path, char** path, char** part) const
{
  if(full_path == NULL)
    return false;

  *path = extractsPath(full_path);
  *part = extractsPart(full_path);

  return true;
}

bool VfsSyscall::isOperationPermitted(Inode* inode, int32 operation)
{
  FileSystem* fs = inode->getFileSystem();

  if( operation == WRITE && (fs->getMountFlags() & MS_RDONLY) )
  {
    debug(VFSSYSCALL, "ERROR - no Writing operation permitted - inode resides on a read-only fs!\n");
    return false;
  }

  // TODO implement UserRights here!

  return true;
}

bool VfsSyscall::diskQuotaNotExceeded()
{
  // TODO implement disk-quota checking here
  return true;
}

Inode* VfsSyscall::resolvePath(FsWorkingDirectory* wd_info, const char* path)
{
  if(path == NULL)
    return NULL;

  debug(VFSSYSCALL, "resolvePath() - going to resolve \"%s\"\n", path);

  // the currently resolved Directory
  Directory* cur_dir = NULL;

  // is absolute path?
  if(path[0] == PATH_SEPARATOR)
  {
    debug(VFSSYSCALL, "resolvePath() - resolving an absolute path\n", path);

    // fetching the root I-Node of the root-filesystem
    cur_dir = getVfsRoot();

    // TODO consider user-specific root-directory
    //FsWorkingDirectory* wd_info = wd_info->getWorkingDirInfo();
    //if(wd_info != NULL)
    //{
    //  cur_dir = root_->getVfsRoot();
    //}

    // path is just the root-directory, that's all
    if(strlen(path) == 1)
    {
      return cur_dir;
    }

  }
  // path is relative, so start off with the relative path
  else if(wd_info)
  {
    debug(VFSSYSCALL, "resolvePath() - resolving a relate path\n", path);

    // in case of failure pick the VFS-root
    if(wd_info != NULL)
    {
      cur_dir = resolveDirectory(NULL, wd_info->getWorkingDirPath());
    }

    if(strlen(path) == 0)
    {
      // empty path means the working-directory
      return cur_dir;
    }
  }

  // no starting point could be resolved, just take the VFS-root Directory
  if(cur_dir == NULL)
    cur_dir = getVfsRoot();

  // split path into tokens
  char* path_cpy = strdup(path);
  char* path_token = strtok(path_cpy, "/");
  debug(VFSSYSCALL, "resolvePath() - duplicated path is \"%s\"\n", path_cpy);

  while(true)
  {
    debug(VFSSYSCALL, "resolvePath() - current path token is \"%s\"\n", path_token);

    // check if the calling thread is allowed to scan the directory
    if(wd_info != NULL && !this->isOperationPermitted(cur_dir, EXECUTE))
    {
      debug(VFSSYSCALL, "resolvePath() - ERROR calling thread is not allowed to read Directory!\n", path_token);

      delete[] path_cpy;
      return NULL;
    }

    // the FileSystem of the current-directory
    FileSystem* cur_dir_fs = cur_dir->getFileSystem();

    // looking up the child inode of the directory:
    Inode* child = cur_dir_fs->lookup(cur_dir, path_token);

    // release Directory I-Node
    cur_dir_fs->releaseInode(cur_dir);

    if(child == NULL)
    {
      // failed to resolve path!
      debug(VFSSYSCALL, "resolvePath() - failed to resolve path child==NULL\n");

      delete[] path_cpy;
      return NULL;
    }

    // get next token
    path_token = strtok(NULL, "/");

    // no next path token available
    if(path_token == NULL || strcmp(path_token, "") == 0)
    {
      // just return the last resolved I-Node
      debug(VFSSYSCALL, "resolvePath() - path was successfully resolved, inode-id=%d\n", child->getID());

      delete[] path_cpy;
      return child;
    }
    // continue to resolve the path
    else
    {
      // child has to be a Directory, otherwise it would not
      // make sense to continue path resolving
      if(child->getType() != Inode::InodeTypeDirectory)
      {
        debug(VFSSYSCALL, "resolvePath() - a part in between the path was not a Directory %d\n", child->getID());

        // decrement reference counter of child-node
        child->getFileSystem()->releaseInode(child);
        delete[] path_cpy;
        return NULL;
      }

      // update cur_dir
      cur_dir = static_cast<Directory*>(child);
    }
  }

  delete[] path_cpy;
  // should not be reached
  return NULL;
}

Directory* VfsSyscall::resolveDirectory(FsWorkingDirectory* wd_info, const char* path)
{
  // resolving Path and casting result into a Directory
  Inode* node = resolvePath(wd_info, path);
  if(node == NULL)
    return NULL;

  if(node->getType() != Inode::InodeTypeDirectory)
  {
    // release acquired but now unused I-Node!
    node->getFileSystem()->releaseInode(node);
    debug(VFSSYSCALL, "resolveFile() - ERROR resolved Inode is not a Directory\n");
    return NULL;
  }

  Directory* dir = static_cast<Directory*>(node);

  // just do a dummy lookup in order to load the Directorie's children
  // if they are not actually loaded
  dir->getFileSystem()->lookup(dir, NULL);

  return dir;
}

File* VfsSyscall::resolveFile(FsWorkingDirectory* wd_info, const char* path)
{
  // resolving Path and casting result into a Directory
  Inode* node = resolvePath(wd_info, path);
  if(node == NULL)
    return NULL;

  if(node->getType() != Inode::InodeTypeFile && !(node->getType() & Inode::InodeTypeFile))
  {
    // release acquired but now unused I-Node!
    node->getFileSystem()->releaseInode(node);
    debug(VFSSYSCALL, "resolveFile() - ERROR resolved Inode is not a File\n");
    return NULL;
  }

  return static_cast<File*>(node);
}

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS

Directory* VfsSyscall::resolveRealDirectory(FsWorkingDirectory* wd_info, const char* path)
{
  debug(VFSSYSCALL, "resolveRealDirectory - CALL\n");

  // resolve the real-Directory under the path

  // 1. split path into first and last part
  char* mnt_parent;
  char* mnt_folder;
  splitPath(path, &mnt_parent, &mnt_folder);

  // 2. resolve the first part of the path as usual
  Directory* parent = resolveDirectory(wd_info, mnt_parent);

  if(parent == NULL)
  {
    debug(VFSSYSCALL, "resolveRealDirectory - ERROR failed to resolve first-part.\n");

    delete[] mnt_parent;
    delete[] mnt_folder;
    return NULL;
  }

  // lock the parent
  parent->getLock()->acquireWriteBlocking();

  // request the real child Inode from the parent
  Inode* inode = parent->getRealInode( mnt_folder );

  // release lock
  parent->getLock()->releaseWrite();
  parent->getFileSystem()->releaseInode( parent );

  delete[] mnt_parent;
  delete[] mnt_folder;

  if(inode == NULL || inode->getType() != Inode::InodeTypeDirectory)
  {
    debug(VFSSYSCALL, "resolveRealDirectory - ERROR failed to resolve child Directory.\n");

    inode->getFileSystem()->releaseInode(inode);
    return NULL;
  }

  // getting the mount-path
  return static_cast<Directory*>(inode);
}

int32 VfsSyscall::mount( FsWorkingDirectory* wd_info, const char *device_name,
                         const char *dir_name, const char* file_system_name __attribute__((unused)),
                         int32 mnt_flags )
{
  debug(VFSSYSCALL, "mount - mounting \"%s\" under \"%s\"\n", device_name, dir_name);

  // special use of mount(), not supported (yet!)
  if(mnt_flags & MS_BIND)
  {
    debug(VFSSYSCALL, "mount - ERROR sorry MS_BIND flag is not supported (yet)!\n");
    return -1;
  }
  else if(mnt_flags & MS_MOVE)
  {
    debug(VFSSYSCALL, "mount - ERROR sorry MS_MOVE flag is not supported (yet)!\n");
    return -1;
  }

  // mounting a new FileSystem to the VFS-Tree

  // getting the device-object by the given DevName to mount
  BDVirtualDevice* bddev = BDManager::getInstance()->getDeviceByName(device_name);

  if(bddev == NULL)
  {
    // device not available
    return -1; //
  }

  // 1. resolve given mount-path
  Directory* mount_path = resolveRealDirectory(wd_info, dir_name);

  if(mount_path == NULL)
  {
    // invalid mount-path, failed to resolve
    debug(VFSSYSCALL, "mount - ERROR failed to resolve given mount-path.\n");
    return -1; // error code
  }

  // create a FsDevice (Adapter class) from the given BDVirtualDevice
  FsDevice* fs_device = new FsDeviceVirtual(bddev);

  // create a new file-system instance that is located on the given device
  FileSystem* mounted_fs = fs_pool_.getNewFsInstance(fs_device, bddev->getPartitionType(), mnt_flags);

  // failed to create the FileSystem instance
  if(mounted_fs == NULL)
  {
    debug(VFSSYSCALL, "mount - ERROR failed to instantiate new file-system\n");
    return -1; //
  }

  debug(VFSSYSCALL, "mount - new fs instance created (%s)\n", mounted_fs->getName());

  mount_path->getLock()->acquireWriteBlocking();

  // mark the Directory object representing the mount-path as a Mount-Point
  // from now on, if the mount-path is resolve instead of the Directory the root
  // Dir of the new created FileSystem will be returned
  // to prevent the loss of the Mount flag in the mount_path Directory the object
  // reference will not be released until umount is called
  mount_path->pushMountedFs(mounted_fs);

  mount_path->getLock()->releaseWrite();

  // add FileSystem to mount-list
  mount_lock_.acquire("VfsSyscall::mount - acquire - add currently mounted FS");
  mounted_fs_.push_back(mounted_fs);
  mount_lock_.release("VfsSyscall::mount - release - add currently mounted FS");

  debug(VFSSYSCALL, "mount - mounting new FileSystem was successful!\n");
  return 0;
}
#endif

#ifndef USE_FILE_SYSTEM_ON_GUEST_OS
int32 VfsSyscall::umount ( FsWorkingDirectory* wd_info, const char *dir_name, int32 flag __attribute__((unused)) )
{
  if(dir_name == NULL)
    return -1; // ERROR

  debug(VFSSYSCALL, "umount - going to unmount fs under \"%s\"\n", dir_name);

  // getting the mount-path
  Directory* mount_path = resolveRealDirectory(wd_info, dir_name);

  // there are now some write - operations following, so lock exclusively
  mount_path->getLock()->acquireWriteBlocking();

  if(!mount_path->isMountPoint())
  {
    debug(VFSSYSCALL, "umount - ERROR given path does not refer to a mount-point.\n");

    mount_path->getLock()->releaseWrite();
    mount_path->getFileSystem()->releaseInode(mount_path);
    return -1;
  }

  mount_lock_.acquire("VfsSyscall::umount - acquire");

  // remove FileSystem from List
  for(uint32 i = 0; i < mounted_fs_.size(); i++)
  {
    if(mounted_fs_[i] == mount_path->getMountedFs())
    {
      assert( mount_path->popMountedFs() );
      delete mounted_fs_[i];
      mounted_fs_.erase(mounted_fs_.begin() + i);
      break;
    }
  }

  mount_lock_.release("VfsSyscall::umount - acquire");
  mount_path->getLock()->releaseWrite();

  // release the mount-point twice, because it was acquired by the
  mount_path->getFileSystem()->releaseInode(mount_path);
  mount_path->getFileSystem()->releaseInode(mount_path);

  debug(VFSSYSCALL, "umount - un-mounting was successful!\n");
  return 0;
}
#endif

unix_time_stamp VfsSyscall::getCurrentTimeStamp(void)
{
#ifdef USE_FILE_SYSTEM_ON_GUEST_OS
  return time(NULL);
#endif

  // TODO implement unix time-stamp here
  return 0;
}

Directory* VfsSyscall::getVfsRoot(void)
{
  //assert(root_);
  return root_->getRoot();
}

char VfsSyscall::getLastDefinedPathSeparator(void)
{
  return LAST_DEFINED_PATH_SEPARATOR;
}

char VfsSyscall::getPathSeparator(void) const
{
  return PATH_SEPARATOR;
}

void VfsSyscall::rewriteParentDirectory(FileSystem* fs, Directory* parent) const
{
  // queue a rewrite-operation for the parent-Directory
  fs->writeInode(parent);

  // if MS_SYNCHRONOUS is set ALL write-operations are synchronous in that
  // case there is no need to sync the given inode
  // otherwise if MS_DIRSYNC flag is set, the Directory should be synced
  if(!(fs->getMountFlags() & MS_SYNCHRONOUS ) && (fs->getMountFlags() & MS_DIRSYNC))
  {
    fs->fsync(parent);
  }
}
