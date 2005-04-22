//----------------------------------------------------------------------
//   $Id: new.cpp,v 1.1 2005/04/22 17:21:41 nomenquis Exp $
//----------------------------------------------------------------------
//
//  $Log: $
//----------------------------------------------------------------------

#include "kmalloc.h"
#include "new.h"

// overloaded normal new operator
void* operator new(size_t size)
{
    // maybe we could take some precautions not to be interrupted while doing this
    void* p = kmalloc(size);
    return p;
}

// overloaded normal delete
void operator delete(void* address)
{
    kfree(address);
    return;
}

// overloaded array new operator
void* operator new[](size_t size)
{
    void* p = kmalloc(size);
    return p;
}

// overloaded array delete operator
void operator delete[](void* address)
{
    kfree(address);
    return;
}

extern "C" void __cxa_pure_virtual();
extern "C" void _pure_virtual(void);
extern "C" void __pure_virtual(void);
extern "C" int atexit( void (*func)(void));
extern "C" int __cxa_atexit();
extern "C" void* __dso_handle;
/*!
    \brief dummy

    dummy

    \author HigePon
    \date   create:2002/08/04 update:2002/02/25
*/
void __cxa_pure_virtual() {

//  g_console->printf("__cxa_pure_virtual called\n");
}

/*!
    \brief dummy

    dummy

    \author HigePon
    \date   create:2002/09/07 update: 2003/02/25
*/
void _pure_virtual() {

//  g_console->printf("_pure_virtual called\n");
}

void __pure_virtual() {

//  g_console->printf("_pure_virtual called\n");
}

/*!
    \brief dummy

    dummy

    \author HigePon
    \date   create:2002/08/08 update:2002/02/25
*/
int atexit( void (*)(void)) {return -1;}

/*!
    \brief dummy for gcc3.3

    dummy

    \author HigePon
    \date   create:2003/10/13 update:
*/
int __cxa_atexit() {return -1;}

#ifndef GCC29
void*   __dso_handle = (void*) &__dso_handle;
#endif
