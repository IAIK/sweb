#pragma once

#include "../../../common/include/kernel/syscall-definitions.h"
#include "types.h"

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
 int createprocess(const char* path, int sleep);

 int getcpu(size_t *cpu, size_t *node, void *tcache);

 ssize_t getdents(int fd, char* buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif
