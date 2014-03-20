//-------------------------------------------------------------------------------------*/
#ifndef BACKTRACE_H_
#define BACKTRACE_H_
//-------------------------------------------------------------------------------------*/
uint64 parse_leb128(uint8*& data);
void parse_dwarf();
class Thread;
struct StabEntry;
int backtrace(pointer *call_stack, int size, Thread *thread,
    bool use_stored_registers);
pointer get_function_name(pointer address, char function_name[]);
void parse_symtab(StabEntry* stab_start, StabEntry* stab_end, const char *stab_str);
//-------------------------------------------------------------------------------------*/
#endif // BACKTRACE_H_
