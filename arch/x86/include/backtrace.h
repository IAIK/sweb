//-------------------------------------------------------------------------------------*/
#ifndef BACKTRACE_H_
#define BACKTRACE_H_
//-------------------------------------------------------------------------------------*/
class Thread;

namespace ustl
{
  class string;
};

struct StabEntry;
//-------------------------------------------------------------------------------------*/
#define ADDRESS_BETWEEN(Value, LowerBound, UpperBound) \
  ((((void*)Value) >= ((void*)LowerBound)) && (((void*)Value) < ((void*)UpperBound)))
//-------------------------------------------------------------------------------------*/
int backtrace(pointer *call_stack, int size, Thread *thread,
    bool use_stored_registers);

pointer get_function_name(pointer address, char function_name[]);

void demangle_name(const char* name, char *buffer);
void parse_symtab(StabEntry* stab_start, StabEntry* stab_end, const char *stab_str);
//-------------------------------------------------------------------------------------*/
#endif // BACKTRACE_H_
