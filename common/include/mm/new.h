#pragma once

#include "types.h"

/**
 * wrapper for placement new operator
 */
[[nodiscard]] inline void* operator new(size_t, void* __p) noexcept;
[[nodiscard]] inline void* operator new[](size_t, void* __p) noexcept;

void __builtin_delete(void* address);
void* __builtin_new(size_t size);
void* __builtin_vec_new(size_t size);
void __builtin_vec_delete(void* address);
