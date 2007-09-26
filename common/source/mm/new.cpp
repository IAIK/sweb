/**
 * @file new.cpp
 */

#include "new.h"
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
  return p;
}

/**
 * overloaded normal delete
 * @param address the address of the memory to delete
 */
void operator delete ( void* address )
{
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
  return p;
}

/**
 * overloaded array delete operator
 * @param address the address of the array to delete
 */
void operator delete[] ( void* address )
{
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
//  g_console->printf("__cxa_pure_virtual called\n");
}

void _pure_virtual()
{
//  g_console->printf("_pure_virtual called\n");
}

void __pure_virtual()
{
//  g_console->printf("_pure_virtual called\n");
}

uint32 atexit ( void ( * ) ( void ) ) {return ( uint32 )-1;}

uint32 __cxa_atexit() {return ( uint32 )-1;}

#ifndef GCC29
void*   __dso_handle = ( void* ) &__dso_handle;
#endif
