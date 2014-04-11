//-------------------------------------------------------------------------------------*/
#include "kprintf.h"
#include "Thread.h"
#include "backtrace.h"
#include "InterruptUtils.h"
#include "ArchThreads.h"
#include "mm/KernelMemoryManager.h" // for use of "kernel_end_address"
#include "ustl/umap.h"
#include "ArchCommon.h"

//-------------------------------------------------------------------------------------*/
#define N_GSYM  0x20    /* global symbol: name,,0,type,0 */
#define N_FNAME 0x22    /* procedure name (f77 kludge): name,,0 */
#define N_FUN   0x24    /* procedure: name,,0,linenumber,address */
#define N_STSYM 0x26    /* static symbol: name,,0,type,address */
#define N_LCSYM 0x28    /* .lcomm symbol: name,,0,type,address */
#define N_RSYM  0x40    /* register sym: name,,0,type,register */
#define N_SLINE 0x44    /* src line: 0,,0,linenumber,address */
#define N_SSYM  0x60    /* structure elt: name,,0,type,struct_offset */
#define N_SO    0x64    /* source file name: name,,0,0,address */
#define N_LSYM  0x80    /* local sym: name,,0,type,offset */
#define N_SOL   0x84    /* #included file name: name,,0,0,address */
#define N_PSYM  0xa0    /* parameter: name,,0,type,offset */
#define N_ENTRY 0xa4    /* alternate entry: name,linenumber,address */
#define N_LBRAC 0xc0    /* left bracket: 0,,0,nesting level,address */
#define N_RBRAC 0xe0    /* right bracket: 0,,0,nesting level,address */
#define N_BCOMM 0xe2    /* begin common: name,, */
#define N_ECOMM 0xe4    /* end common: name,, */
#define N_ECOML 0xe8    /* end common (local name): ,,address */
#define N_LENG  0xfe    /* second stab entry with length information */
//-------------------------------------------------------------------------------------*/
struct StackFrame
{
   void *return_address;
   StackFrame *previous_frame;
};

struct StabEntry
{
    uint32 n_strx;
    uint8 n_type;
    uint8 n_other;
    uint16 n_desc;
    uint32 n_value;
} __attribute__((packed));
//-------------------------------------------------------------------------------------*/
extern Thread* currentThread;
ustl::map<uint32, const char*> symbol_table;
//-------------------------------------------------------------------------------------*/
bool try_paste_operator(const char *&input, char *& buffer);
int read_number(const char *& input);
void paste_typename(const char *& input, char *& buffer);
//-------------------------------------------------------------------------------------*/

bool try_paste_operator(const char *& input, char *& buffer)
{
  if (!input || !buffer) return false;

  char const *OperatorName;

  if (input[0] == 'n' && input[1] == 'w')
    OperatorName = " new";
  else if (input[0] == 'n' && input[1] == 'a')
    OperatorName = " new[]";
  else if (input[0] == 'd' && input[1] == 'l')
    OperatorName = " delete";
  else if (input[0] == 'd' && input[1] == 'a')
    OperatorName = " delete[]";
  else if (input[0] == 'p' && input[1] == 's')
    OperatorName = "+";
  else if (input[0] == 'n' && input[1] == 'g')
    OperatorName = "-";
  else if (input[0] == 'a' && input[1] == 'd')
    OperatorName = "&";
  else if (input[0] == 'd' && input[1] == 'e')
    OperatorName = "*";
  else if (input[0] == 'c' && input[1] == 'o')
    OperatorName = "~";
  else if (input[0] == 'p' && input[1] == 'l')
    OperatorName = "+";
  else if (input[0] == 'm' && input[1] == 'i')
    OperatorName = "-";
  else if (input[0] == 'm' && input[1] == 'l')
    OperatorName = "*";
  else if (input[0] == 'd' && input[1] == 'v')
    OperatorName = "/";
  else if (input[0] == 'r' && input[1] == 'm')
    OperatorName = "%";
  else if (input[0] == 'a' && input[1] == 'n')
    OperatorName = "&";
  else if (input[0] == 'o' && input[1] == 'r')
    OperatorName = "|";
  else if (input[0] == 'e' && input[1] == 'o')
    OperatorName = "^";
  else if (input[0] == 'a' && input[1] == 'S')
    OperatorName = "=";
  else if (input[0] == 'p' && input[1] == 'L')
    OperatorName = "+=";
  else if (input[0] == 'm' && input[1] == 'I')
    OperatorName = "-=";
  else if (input[0] == 'm' && input[1] == 'L')
    OperatorName = "*=";
  else if (input[0] == 'd' && input[1] == 'V')
    OperatorName = "/=";
  else if (input[0] == 'r' && input[1] == 'M')
    OperatorName = "%=";
  else if (input[0] == 'a' && input[1] == 'N')
    OperatorName = "&=";
  else if (input[0] == 'o' && input[1] == 'R')
    OperatorName = "|=";
  else if (input[0] == 'e' && input[1] == 'O')
    OperatorName = "^=";
  else if (input[0] == 'l' && input[1] == 's')
    OperatorName = "<<";
  else if (input[0] == 'r' && input[1] == 's')
    OperatorName = ">>";
  else if (input[0] == 'l' && input[1] == 'S')
    OperatorName = "<<=";
  else if (input[0] == 'r' && input[1] == 'S')
    OperatorName = ">>=";
  else if (input[0] == 'e' && input[1] == 'q')
    OperatorName = "==";
  else if (input[0] == 'n' && input[1] == 'e')
    OperatorName = "!=";
  else if (input[0] == 'l' && input[1] == 't')
    OperatorName = "<";
  else if (input[0] == 'g' && input[1] == 't')
    OperatorName = ">";
  else if (input[0] == 'l' && input[1] == 'e')
    OperatorName = "<=";
  else if (input[0] == 'g' && input[1] == 'e')
    OperatorName = ">=";
  else if (input[0] == 'n' && input[1] == 't')
    OperatorName = "!";
  else if (input[0] == 'a' && input[1] == 'a')
    OperatorName = "&&";
  else if (input[0] == 'o' && input[1] == 'o')
    OperatorName = "||";
  else if (input[0] == 'p' && input[1] == 'p')
    OperatorName = "++";
  else if (input[0] == 'm' && input[1] == 'm')
    OperatorName = "--";
  else if (input[0] == 'c' && input[1] == 'm')
    OperatorName = ",";
  else if (input[0] == 'p' && input[1] == 'm')
    OperatorName = "->*";
  else if (input[0] == 'p' && input[1] == 't')
    OperatorName = "->";
  else if (input[0] == 'c' && input[1] == 'l')
    OperatorName = "()";
  else if (input[0] == 'i' && input[1] == 'x')
    OperatorName = "[]";
  else if (input[0] == 'q' && input[1] == 'u')
    OperatorName = "?";
  else if ((input[0] == 's' && input[1] == 't') || (input[0] == 's' && input[1] == 'z'))
    OperatorName = "sizeof";
  /* else if (input[0] == 'c' && input[1] == 'v')
    OperatorName = "<type> # (cast))"; */
  else
    return false;

  int n = strlen(OperatorName);

  memcpy(buffer, "operator", 8);
  buffer += 8;
  memcpy(buffer, OperatorName, n);
  buffer += n;
  input += 2;
  return true;
}
//-------------------------------------------------------------------------------------*/

int backtrace(pointer *call_stack, int size, Thread *thread, bool use_stored_registers)
{
  if (!call_stack ||
      (use_stored_registers && !thread) ||
      (!use_stored_registers && thread != currentThread) ||
      size <= 1)
    return 0;

  void *fp = 0;

  if (!use_stored_registers)
  {
    asm("mov %[v], fp" : [v]"=r" (fp));
  }
  else
    fp = (void*)thread->kernel_arch_thread_info_->r11; // r11 is the fp register in gcc ;)

    int i = 0;
  StackFrame *CurrentFrame = (StackFrame*)(fp-8);
  void *StackStart = (void*)((uint32)thread->stack_ + sizeof(thread->stack_)); // the stack "starts" at the high addresses...
  void *StackEnd = (void*)thread->stack_; // ... and "ends" at the lower ones.

  if (use_stored_registers)
    call_stack[i++] = thread->kernel_arch_thread_info_->pc;


  void *StartAddress = (void*)0x80000000;
  void *EndAddress = (void*)ArchCommon::getFreeKernelMemoryEnd();

  while (i < size &&
      ADDRESS_BETWEEN(CurrentFrame, StackEnd, StackStart) &&
      ADDRESS_BETWEEN(CurrentFrame->return_address, StartAddress, EndAddress) &&
      ADDRESS_BETWEEN(StackEnd, StartAddress, EndAddress) &&
      ADDRESS_BETWEEN(StackStart, StartAddress, EndAddress))
  {
    call_stack[i++] = (pointer)CurrentFrame->return_address;
    CurrentFrame = CurrentFrame->previous_frame;
  }

  return i;
}
//-------------------------------------------------------------------------------------*/

pointer get_function_name(pointer address, char function_name[])
{
  if (symbol_table.size() == 0)
    return NULL;
  ustl::map<uint32, const char*>::iterator it = symbol_table.end();

  if (ADDRESS_BETWEEN(address, symbol_table.at(0).first, &kernel_end_address))
  {
    it = symbol_table.begin();

    while (it != symbol_table.end() && it->first <= address)
      ++it;
  }

  if (it != symbol_table.end())
  {
    --it;
    demangle_name(it->second, function_name);
    return it->first;
  }

  return NULL;
}
//-------------------------------------------------------------------------------------*/

int read_number(const char *& input)
{
  if (!input) return -1;

  int Result = 0;

  while (*input >= '0' && *input <= '9')
  {
    Result = 10*Result + (*input - '0');
    ++input;
  }

  return Result;
}
//-------------------------------------------------------------------------------------*/

void paste_typename(const char *& input, char *& buffer)
{
  bool IsReference = false, IsPointer = false;
  int OffsetInput, Count;
  char const *Src;

  if (!input || !buffer) return;

  if (*input == 'R')
  {
    ++input;
    IsReference = true;
  }

  if (*input == 'P')
  {
    ++input;
    IsPointer = true;
  }

  if (*input == 'K')
  {
    memcpy(buffer, "const ", 6);
    ++input;
    buffer += 6;
  }

  if (*input >= '0' && *input <= '9')
  {
    Count = read_number(input);
    OffsetInput = Count;
    Src = input;
  }
  else
  {
    switch (*input)
    {
    case 'v':
      Src = "";
      break;

    case 'w':
      Src = "wchar_t";
      break;

    case 'b':
      Src = "bool";
      break;

    case 'c':
      Src = "char";
      break;

    case 'a':
      Src = "signed char";
      break;

    case 'h':
      Src = "unsigned char";
      break;

    case 's':
      Src = "short";
      break;

    case 't':
      Src = "unsigned short";
      break;

    case 'i':
      Src = "int";
      break;

    case 'j':
      Src = "unsigned int";
      break;

    case 'l':
      Src = "long";
      break;

    case 'm':
      Src = "unsigned long";
      break;

    case 'x':
      Src = "long long";
      break;

    case 'y':
      Src = "unsigned long long";
      break;

    case 'n':
      Src = "__int128";
      break;

    case 'o':
      Src = "unsigned __int128";
      break;

    case 'f':
      Src = "float";
      break;

    case 'd':
      Src = "double";
      break;

    case 'e':
      Src = "long double";
      break;

    case 'g':
      Src = "__float128";
      break;

    case 'z':
      Src = "ellipsis";
      break;

    default:
      Src = "<?>";
      break;
    }

    Count = strlen(Src);
    OffsetInput = 1;
  }

  memcpy(buffer, Src, Count);
  buffer += Count;
  input += OffsetInput;

  if (IsPointer)
    *buffer++ = '*';

  if (IsReference)
    *buffer++ = '&';
}
//-------------------------------------------------------------------------------------*/

void paste_arguments(const char *& input, char *& buffer, char delimiter)
{
  if (!input || !buffer) return;

  int ArgNr = 0;

  while (*input != delimiter)
  {
    if (ArgNr++)
    {
      *buffer++ = ',';
      *buffer++ = ' ';
    }

    paste_typename(input, buffer);
  }
}
//-------------------------------------------------------------------------------------*/

void demangle_name(const char* name, char *buffer)
{
  const char *pData = name;

  if (!buffer || !pData) return;

  if (pData[0] != '_' || pData[1] != 'Z')  // is the name mangled?
  {
    size_t length = strlen(name);
    for (size_t i = 0; i < length; ++i) // copy unmangled name
    {
      if (*pData == ':') break;

      *buffer++ = *pData++;
    }

    *buffer = '\0';
    return;
  }

  pData += 2; // skip "_Z", which indicates a mangled name

  if (*pData == 'N') // we've got a nested name
  {
    ++pData;
    int NameCount = 0;

    uint32 repeat = 0;
    while (*pData != 'E')
    {
      if (NameCount++ && *pData != 'I')
      {
        *buffer++ = ':';
        *buffer++ = ':';
      }

      if (*pData >= '0' && *pData <= '9') // part of nested name
      {
        int Count = read_number(pData);

        memcpy(buffer, pData, Count);
        pData += Count;
        buffer += Count;
      }
      else if (*pData == 'I') // parse template params
      {
        *buffer++ = '<';
        paste_arguments(++pData, buffer, 'E');
        *buffer++ = '>';
        ++pData;
      }
      else
        try_paste_operator(pData, buffer);
      if (repeat >= 2)
        break;
      ++repeat;
    }
    ++pData;
  }
  else
  {
    if (*pData == 'L') // we've got a local name
      ++pData;

    int Count = read_number(pData);
    memcpy(buffer, pData, Count);
    buffer += Count;
    pData += Count;
  }

  *buffer++ = '(';
  paste_arguments(pData, buffer, ':');
  *buffer++ = ')';
  *buffer = '\0';
}
//-------------------------------------------------------------------------------------*/

void parse_symtab(StabEntry *stab_start, StabEntry *stab_end, const char *stab_str)
{
  new (&symbol_table) ustl::map<uint32, const char*>();
  symbol_table.reserve(2048);

  for (StabEntry* current_stab = stab_start; current_stab < stab_end; ++current_stab)
  {
    if (unlikely((current_stab->n_type == N_FUN || current_stab->n_type == N_FNAME) &&
        ADDRESS_BETWEEN(current_stab->n_value, 0x80000000, &kernel_end_address)))
    {
      symbol_table[current_stab->n_value] = stab_str+current_stab->n_strx;
    }
  }
  debug(MAIN, "found %d functions\n", symbol_table.size());
}
//-------------------------------------------------------------------------------------*/
