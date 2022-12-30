// WARNING: You are looking for a different ScopeLock.h - this one is just for the exe2minixfs tool!

#ifdef EXE2MINIXFS
#pragma once

#include "types.h"

class Mutex;

class ScopeLock
{
public:
    ScopeLock(Mutex&, bool = true, pointer = 0) {}

    ScopeLock(const ScopeLock&) = delete;
    ScopeLock& operator=(const ScopeLock&) = delete;
};
#endif
