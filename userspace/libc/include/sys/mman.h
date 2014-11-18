#ifndef mman_h___
#define mman_h___

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif


#define PROT_NONE     0x00000000  // 00..00
#define PROT_READ     0x00000001  // ..0001
#define PROT_WRITE    0x00000002  // ..0010

#define MAP_PRIVATE   0x00000000  // 00..00
#define MAP_SHARED    0x40000000  // 0100..
#define MAP_ANONYMOUS 0x80000000  // 1000..

extern void* mmap(void* start, size_t length, int prot, int flags, int fd, off_t offset);

extern int munmap(void* start, size_t length);

extern int shm_open(const char* name, int oflag, mode_t mode);

extern int shm_unlink(const char* name);

#ifdef __cplusplus
}
#endif

#endif // mman_h___


