#include "new.h"
#include "assert.h"
#include "KernelMemoryManager.h"
#include "backtrace.h"
#include "debug.h"
#include "stdint.h"
#include "ArchInterrupts.h"
#include "SMP.h"
#include "ArchMulticore.h"
#include "Scheduler.h"
#include "SystemState.h"

/**
 * delete (free) memory. This function is used by the wrappers.
 * @param address the address of the memory to delete
 */
static void _delete(void* address)
{
    if (address) {
        address = ((uint8*)address) - 0x10;
    }

    pointer called_by = getCalledBefore(2);
    debug(KMM, "delete %p\n", address);
    assert(address > (void*)0x80000000 || address == (void*)nullptr);
    KernelMemoryManager::instance()->freeMemory ( ( pointer ) address, called_by);
}

/**
 * Allocate new memory. This function is used by the wrappers.
 * @param size the size of the memory to allocate
 * @return the pointer to the new memory
 */
__attribute__((__alloc_size__ (1), __malloc__, __malloc__(_delete, 1)))
static void* _new(size_t size, pointer called_by = getCalledBefore(1))
{
  debug(KMM, "new, size: %zu\n", size);
  // maybe we could take some precautions not to be interrupted while doing this
  void* p = ( void* ) KernelMemoryManager::instance()->allocateMemory (size + 0x10, called_by);
  assert(p > (void*)0x80000000 || p == (void*)nullptr);
  if (p) {
    *static_cast<size_t *>(p) = size;
    p = ((uint8 *)p) + 0x10;
  }
#if !DYNAMIC_KMM
  assert(p);
#endif
  return p;
}

/**
 * overloaded normal new operator
 * @param size the size of the memory to allocate
 * @return the pointer to the new memory
 */
__attribute__((__alloc_size__ (1), __malloc__, __malloc__(_delete, 1)))
void* operator new ( size_t size )
{
  return _new(size);
}

/**
 * overloaded normal delete
 * @param address the address of the memory to delete
 */
void operator delete ( void* address )
{
  _delete(address);
}

void operator delete(void* address, size_t size)
{
  if (address) {
    auto new_size = *(size_t *)(((uint8 *)address) - 0x10);
    if (new_size != size)
    {
        debugAlways(KMM, "Invalid delete call. delete and new size do not match. Expected size: %zu, delete size: %zu\n", new_size, size);
    }
    assert(new_size == size && "Invalid delete call. delete and new size do not match.");
  }
  _delete(address);
}

/**
 * overloaded array new operator
 * @param size the size of the array to allocate
 * @return the pointer to the new memory
 */
__attribute__((__alloc_size__ (1), __malloc__, __malloc__(_delete, 1)))
void* operator new[] ( size_t size )
{
  return _new(size);
}

/**
 * overloaded array delete operator
 * @param address the address of the array to delete
 */
void operator delete[] ( void* address )
{
  _delete(address);
}

void operator delete[](void* address, size_t size)
{
  if (address) {
    auto new_size = *(size_t *)(((uint8 *)address) - 0x10);
    if (new_size != size)
    {
        debugAlways(KMM, "Invalid delete call. delete and new size do not match. Expected size: %zu, delete size: %zu\n", new_size, size);
    }
    assert(new_size == size && "Invalid delete call. delete and new size do not match.");
  }
  _delete(address);
}

extern "C" void __cxa_pure_virtual();
extern "C" void _pure_virtual();
extern "C" void __pure_virtual();
extern "C" uint32 atexit ( void ( *func )() );
extern "C" uint32 __cxa_atexit();
extern void* __dso_handle;

void __cxa_pure_virtual()
{
    assert(false);
}

void _pure_virtual()
{
}

void __pure_virtual()
{
}

uint32 atexit ( void ( * )() ) {return ( uint32 )-1;}

uint32 __cxa_atexit() {return ( uint32 )-1;}

#ifndef GCC29
void*   __dso_handle = ( void* ) &__dso_handle;
#endif

void checkKMMDeadlock(const char* pName = nullptr, const char* file = nullptr, int line = 0)
{
    if (SMP::numRunningCpus() == 1 && unlikely (ArchInterrupts::testIFSet() == false || Scheduler::instance()->isSchedulingEnabled() == false))
    {
        if (unlikely (KernelMemoryManager::instance()->KMMLockHeldBy() != nullptr))
        {
            system_state = KPANIC;
            kprintfd("(ERROR) checkKMMDeadlock: Using a non resize-safe container method with IF=%d and SchedulingEnabled=%d ! This will fail!!!\n"
                     "        container: %s, called at: %s, line: %d\n",
                     ArchInterrupts::testIFSet(), Scheduler::instance()->isSchedulingEnabled(), pName ? pName : "(nil)", file ? file : "(nil)", line);
            currentThread->printBacktrace(true);
            assert(false);
        }
    }
}

// Required for EASTL
void* operator new[](size_t size, [[maybe_unused]] const char* pName, [[maybe_unused]] int flags, [[maybe_unused]] unsigned debugFlags, [[maybe_unused]] const char* file, [[maybe_unused]] int line)
{
    checkKMMDeadlock(pName, file, line);
    // Number of call frames to go up the stack to get to the actually location where allocation happened in user code varies depending on container, etc... 6 is enough to get the actual call site for vector::emplace_back() and is close enough for other containers to get useful function information instead of just allocator::allocate
    return _new(size, getCalledBefore(6));
}

void* operator new[](size_t size, [[maybe_unused]] size_t alignment, [[maybe_unused]] size_t alignmentOffset, [[maybe_unused]] const char* pName, [[maybe_unused]] int flags, [[maybe_unused]] unsigned debugFlags, [[maybe_unused]] const char* file, [[maybe_unused]] int line)
{
    // TODO: respect alignment
    checkKMMDeadlock(pName, file, line);
    return _new(size, getCalledBefore(6));
}
