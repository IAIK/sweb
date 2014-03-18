/**
 * @file the contents of this file are taken from <sys/mount.h> !!!
 *
 * Filename: MountFlags.h
 * Description:
 */

#ifndef MOUNTFLAGS_H_
#define MOUNTFLAGS_H_

/* These are the fs-independent mount-flags: up to 16 flags are
   supported  */
enum
{
  MS_RDONLY = 1,    /* Mount read-only.  */
#define MS_RDONLY MS_RDONLY
  MS_NOSUID = 2,    /* Ignore suid and sgid bits.  */
#define MS_NOSUID MS_NOSUID
  MS_NODEV = 4,     /* Disallow access to device special files.  */
#define MS_NODEV  MS_NODEV
  MS_NOEXEC = 8,    /* Disallow program execution.  */
#define MS_NOEXEC MS_NOEXEC
  MS_SYNCHRONOUS = 16,    /* Writes are synced at once.  */
#define MS_SYNCHRONOUS  MS_SYNCHRONOUS
  MS_REMOUNT = 32,    /* Alter flags of a mounted FS.  */
#define MS_REMOUNT  MS_REMOUNT
  MS_MANDLOCK = 64,   /* Allow mandatory locks on an FS.  */
#define MS_MANDLOCK MS_MANDLOCK
  MS_DIRSYNC = 128,   /* Directory modifications are synchronous.  */
#define MS_DIRSYNC  MS_DIRSYNC
  MS_NOATIME = 1024,    /* Do not update access times.  */
#define MS_NOATIME  MS_NOATIME
  MS_NODIRATIME = 2048,   /* Do not update directory access times.  */
#define MS_NODIRATIME MS_NODIRATIME
  MS_BIND = 4096,   /* Bind directory at different place.  */
#define MS_BIND   MS_BIND
  MS_MOVE = 8192,
#define MS_MOVE   MS_MOVE
  MS_REC = 16384,
#define MS_REC    MS_REC
  MS_SILENT = 32768,
#define MS_SILENT MS_SILENT
  MS_POSIXACL = 1 << 16,  /* VFS does not apply the umask.  */
#define MS_POSIXACL MS_POSIXACL
  MS_UNBINDABLE = 1 << 17,  /* Change to unbindable.  */
#define MS_UNBINDABLE MS_UNBINDABLE
  MS_PRIVATE = 1 << 18,   /* Change to private.  */
#define MS_PRIVATE  MS_PRIVATE
  MS_SLAVE = 1 << 19,   /* Change to slave.  */
#define MS_SLAVE  MS_SLAVE
  MS_SHARED = 1 << 20,    /* Change to shared.  */
#define MS_SHARED MS_SHARED
  MS_RELATIME = 1 << 21,  /* Update atime relative to mtime/ctime.  */
#define MS_RELATIME MS_RELATIME
  MS_KERNMOUNT = 1 << 22, /* This is a kern_mount call.  */
#define MS_KERNMOUNT  MS_KERNMOUNT
  MS_I_VERSION =  1 << 23,  /* Update inode I_version field.  */
#define MS_I_VERSION  MS_I_VERSION
  MS_STRICTATIME = 1 << 24, /* Always perform atime updates.  */
#define MS_STRICTATIME  MS_STRICTATIME
  MS_ACTIVE = 1 << 30,
#define MS_ACTIVE MS_ACTIVE
  MS_NOUSER = 1 << 31
#define MS_NOUSER MS_NOUSER
};


#endif /* MOUNTFLAGS_H_ */
