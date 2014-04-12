//-------------------------------------------------------------------------------------*/
#ifndef BACKTRACE_H_
#define BACKTRACE_H_

#include "arch_backtrace.h"
//-------------------------------------------------------------------------------------*/
class Thread;

struct StabEntry;
//-------------------------------------------------------------------------------------*/
pointer get_function_name(pointer address, char function_name[]);

void demangle_name(const char* name, char *buffer);
void parse_symtab(StabEntry* stab_start, StabEntry* stab_end, const char *stab_str);
//-------------------------------------------------------------------------------------*/
#endif // BACKTRACE_H_
