#ifndef nonstd_h___
#define nonstd_h___

#include "../../../common/include/kernel/syscall-definitions.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a new process.
 *
 * @param path the path to the binary to open
 * @param sleep whether the calling process should sleep until the other process terminated
 * @return -1 if the path did not lead to an executable, 0 if the process was executed successfully
 *
 */ 
extern int createprocess(const char* path, int sleep);

#ifdef __cplusplus
}
#endif

#endif // nonstd_h___


