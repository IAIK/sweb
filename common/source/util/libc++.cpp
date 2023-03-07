#include "EASTL/atomic.h"
#include "assert.h"

extern "C" int __cxa_thread_atexit(__attribute__((unused)) void (*func)(), __attribute__((unused)) void *obj, __attribute__((unused)) void *dso_symbol)
{
        // Required for thread_local keyword (used for cpu local storage). Should technically call destructors of thread local objects (respectively cpu local in this case), but we don't care about that since we don't want to destroy cpu local storage
        return 0;
}


// https://android.googlesource.com/platform/bionic/+/c9a659c/libc/bionic/__cxa_guard.cpp

#if defined(__arm__)
// The ARM C++ ABI mandates that guard variables are 32-bit aligned, 32-bit
// values. The LSB is tested by the compiler-generated code before calling
// __cxa_guard_acquire.
union _guard_t {
    eastl::atomic<int> state;
    int32_t aligner;
};
static_assert(sizeof(_guard_t) == sizeof(int32_t), "Size of static initializer guard struct must be 4 bytes");
#else
// The Itanium/x86 C++ ABI (used by all other architectures) mandates that
// guard variables are 64-bit aligned, 64-bit values. The LSB is tested by
// the compiler-generated code before calling __cxa_guard_acquire.
union _guard_t {
    eastl::atomic<int> state;
    int64_t aligner;
};
static_assert(sizeof(_guard_t) == sizeof(int64_t), "Size of static initializer guard struct must be 8 bytes");
#endif


// Set construction state values according to reference documentation.
// 0 is the initialization value.
// Arm requires ((*gv & 1) == 1) after __cxa_guard_release, ((*gv & 3) == 0) after __cxa_guard_abort.
// X86 requires first byte not modified by __cxa_guard_acquire, first byte is non-zero after
// __cxa_guard_release.
#define CONSTRUCTION_NOT_YET_STARTED                0
#define CONSTRUCTION_COMPLETE                       1
#define CONSTRUCTION_UNDERWAY                   0x100

extern "C" int __cxa_guard_acquire(_guard_t* gv) {
  int old_value = gv->state.load(eastl::memory_order_relaxed);
  while (true) {
    if (old_value == CONSTRUCTION_COMPLETE)
    {
      // A load_acquire operation is need before exiting with COMPLETE state, as we have to ensure
      // that all the stores performed by the construction function are observable on this CPU
      // after we exit.
      eastl::atomic_thread_fence(eastl::memory_order_acquire);
      return 0;
    }
    else if (old_value == CONSTRUCTION_NOT_YET_STARTED)
    {
      // Spinlock via compare exchange
      if (!gv->state.compare_exchange_weak(old_value,
                                           CONSTRUCTION_UNDERWAY)) //,
                                           //eastl::memory_order_release,
                                           //eastl::memory_order_acquire))
      {
        continue;
      }
      // The acquire fence may not be needed. But as described in section 3.3.2 of
      // the Itanium C++ ABI specification, it probably has to behave like the
      // acquisition of a mutex, which needs an acquire fence.
      eastl::atomic_thread_fence(eastl::memory_order_acquire);
      return 1;
    }
  }
}

extern "C" void __cxa_guard_release(_guard_t* gv) {
  gv->state.store(CONSTRUCTION_COMPLETE, eastl::memory_order_release);
}

extern "C" void __cxa_guard_abort(_guard_t* gv) {
  gv->state.store(CONSTRUCTION_NOT_YET_STARTED, eastl::memory_order_release);
}


using func_ptr = void (*)();
extern "C" func_ptr __preinit_array_start;
extern "C" func_ptr __preinit_array_end;
extern "C" func_ptr __init_array_start;
extern "C" func_ptr __init_array_end;
extern "C" func_ptr __fini_array_start;
extern "C" func_ptr __fini_array_end;

void _preinit()
{
    assert(&__preinit_array_start <= &__preinit_array_end);
    for (func_ptr* it = &__preinit_array_start; it < &__preinit_array_end; ++it)
    {
        assert(*it);
        (*it)();
    }
}

void globalConstructors()
{
    assert(&__init_array_start <= &__init_array_end);
    for (func_ptr* it = &__init_array_start; it < &__init_array_end; ++it)
    {
        assert(*it);
        (*it)();
    }
}

void globalDestructors()
{
    for (func_ptr* it = &__fini_array_start; it < &__fini_array_end; ++it)
    {
        assert(*it);
        (*it)();
    }
}
