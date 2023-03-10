#pragma once

#include <types.h>
#include <cstddef>

namespace Syscall
{
    size_t syscallException(size_t syscall_number,
                            size_t arg1,
                            size_t arg2,
                            size_t arg3,
                            size_t arg4,
                            size_t arg5);

    [[noreturn]] void exit(size_t exit_code);
    void outline(size_t port, pointer text);

    size_t write(size_t fd, pointer buffer, size_t size);
    size_t read(size_t fd, pointer buffer, size_t count);
    size_t close(size_t fd);
    size_t open(size_t path, size_t flags);
    size_t lseek(int fd, off_t offset, int whence);
    void pseudols(const char* pathname, char* buffer, size_t size);
    ssize_t getdents(int fd, char* buffer, size_t size);

    size_t createprocess(size_t path, size_t sleep);
    void trace();

    int getcpu(size_t* cpu, size_t* node, void* tcache);
}; // namespace Syscall
