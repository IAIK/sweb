#pragma once

#include "types.h"

void* operator new ( size_t size ) noexcept;

/**
 * wrapper for placement new operator
 */
inline void* operator new(size_t, void* p) noexcept
{
  return p;
}
