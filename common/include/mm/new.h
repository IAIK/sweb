#pragma once

#include "types.h"
#include <new>

void __builtin_delete(void* address);
void* __builtin_new(size_t size);
void* __builtin_vec_new(size_t size);
void __builtin_vec_delete(void* address);
