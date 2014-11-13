#ifndef ARCH_BACKTRACE_H_
#define ARCH_BACKTRACE_H_

#include "types.h"

#define ADDRESS_BETWEEN(Value, LowerBound, UpperBound) \
  ((((void*)(Value)) >= ((void*)(LowerBound))) && (((void*)(Value)) < ((void*)(UpperBound))))

class Thread;

int backtrace(pointer *call_stack, int size, Thread *thread, bool use_stored_registers);
int backtrace_user(pointer *call_stack, int size, Thread *thread, bool use_stored_registers);


#endif // ARCH_BACKTRACE_H_
