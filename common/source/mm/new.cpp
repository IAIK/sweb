#include "new.h"
#include "assert.h"
#include "KernelMemoryManager.h"

/**
 * overloaded normal new operator
 * @param size the size of the memory to allocate
 * @return the pointer to the new memory
 */
void* operator new ( size_t size )
{
  // maybe we could take some precautions not to be interrupted while doing this
  void* p = ( void* ) KernelMemoryManager::instance()->allocateMemory ( size );
  assert(p > (void*)0x80000000 || p == (void*)0);
  return p;
}

/**
 * overloaded normal delete
 * @param address the address of the memory to delete
 */
void operator delete ( void* address )
{
  assert(address > (void*)0x80000000 || address == (void*)0);
  KernelMemoryManager::instance()->freeMemory ( ( pointer ) address );
  return;
}

/**
 * overloaded array new operator
 * @param size the size of the array to allocate
 * @return the pointer to the new memory
 */
void* operator new[] ( size_t size )
{
  void* p = ( void* ) KernelMemoryManager::instance()->allocateMemory ( size );
  assert(p > (void*)0x80000000 || p == (void*)0);
  return p;
}

/**
 * overloaded array delete operator
 * @param address the address of the array to delete
 */
void operator delete[] ( void* address )
{
  assert(address > (void*)0x80000000 || address == (void*)0);
  KernelMemoryManager::instance()->freeMemory ( ( pointer ) address );
  return;
}

extern "C" void __cxa_pure_virtual();
extern "C" void _pure_virtual ( void );
extern "C" void __pure_virtual ( void );
extern "C" uint32 atexit ( void ( *func ) ( void ) );
extern "C" uint32 __cxa_atexit();
extern "C" void* __dso_handle;

void __cxa_pure_virtual()
{
}

void _pure_virtual()
{
}

void __pure_virtual()
{
}

uint32 atexit ( void ( * ) ( void ) ) {return ( uint32 )-1;}

uint32 __cxa_atexit() {return ( uint32 )-1;}

#ifndef GCC29
void*   __dso_handle = ( void* ) &__dso_handle;
#endif
