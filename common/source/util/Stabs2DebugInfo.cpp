#include "kprintf.h"
#include "Stabs2DebugInfo.h"
#include "ArchCommon.h"

#define ADDRESS_BETWEEN(Value, LowerBound, UpperBound) \
  ((((void*)Value) >= ((void*)LowerBound)) && (((void*)Value) < ((void*)UpperBound)))

#define N_FNAME 0x22    /* procedure name (f77 kludge): name,,0 */
#define N_FUN   0x24    /* procedure: name,,0,linenumber,address */
#define N_SLINE 0x44    /* src line: 0,,0,linenumber,address */
#define N_PSYM  0xa0    /* parameter: name,,0,type,offset */

struct StabEntry
{
    uint32 n_strx;
    uint8 n_type;
    uint8 n_other;
    uint16 n_desc;
    uint32 n_value;
}__attribute__((packed));

Stabs2DebugInfo::Stabs2DebugInfo(char const *stab_start, char const *stab_end, char const *stab_str) :
    stab_start_(reinterpret_cast<StabEntry const *>(stab_start)),
    stab_end_(reinterpret_cast<StabEntry const *>(stab_end)), stabstr_buffer_(stab_str)
{
  initialiseSymbolTable();
}

Stabs2DebugInfo::~Stabs2DebugInfo()
{
  // we reinterpret casted this from normal character buffers
  // so make sure we don't execute any potential dtors, no ctors were called anyway
  operator delete[](reinterpret_cast<void*>(const_cast<StabEntry *>(stab_start_)));
  operator delete[](reinterpret_cast<void*>(const_cast<char *>(stabstr_buffer_)));
}

void Stabs2DebugInfo::initialiseSymbolTable()
{
  function_symbols_.reserve(256);

  // debug output for userspace symols
  for (StabEntry const *current_stab = stab_start_; current_stab < stab_end_; ++current_stab)
  {
    if (current_stab->n_type == N_FUN || current_stab->n_type == N_FNAME)
    {
      function_symbols_[current_stab->n_value] = current_stab;
    }
  }
  debug(MAIN, "found %d functions\n", function_symbols_.size());

}

void Stabs2DebugInfo::printAllFunctions() const
{
  char buffer[512];
  debug(MAIN, "Known symbols:\n");
  for (auto symbol : function_symbols_)
  {
    demangleName(stabstr_buffer_ + symbol.second->n_strx, buffer, 512);
    debug(MAIN, "\t%s\n", buffer);
  }
}

struct StabsOperator
{
  const char* mangled;
  const char* demangled;
} operators[] = {
  {"nw"," new"}, {"na"," new[]"}, {"dl"," delete"}, {"da"," delete[]"}, {"ps","+"}, {"ng","-"}, {"ad","&"}, {"de","*"},
  {"co","~"}, {"pl","+"}, {"mi","-"}, {"ml","*"}, {"dv","/"}, {"rm","%"}, {"an","&"}, {"or","|"}, {"eo","^"},
  {"aS","="}, {"pL","+="}, {"mI","-="}, {"mL","*="}, {"dV","/="}, {"rM","%="}, {"aN","&="}, {"oR","|="}, {"eO","^="},
  {"ls","<<"}, {"rs",">>"}, {"lS","<<="}, {"rS",">>="}, {"eq","=="}, {"ne","!="}, {"lt","<"}, {"gt",">"}, {"le","<="},
  {"ge",">="}, {"nt","!"}, {"aa","&&"}, {"oo","||"}, {"pp","++"}, {"mm","--"}, {"cm",","}, {"pm","->*"}, {"pt","->"},
  {"cl","()"}, {"ix","[]"}, {"qu","?"}, {"st","sizeof"}, {"sz","sizeof"}
};

bool Stabs2DebugInfo::tryPasteOoperator(const char *& input, char *& buffer, size_t& size) const
{
  if (!input || !buffer)
    return false;

  const char* operator_name = 0;

  for (StabsOperator op : operators)
  {
    if (input[0] == op.mangled[0] && input[1] == op.mangled[1])
    {
      operator_name = op.demangled;
      break;
    }
  }

  if (operator_name == 0)
    return false;

  size_t n = strlen(operator_name);

  const char* op_str = "operator";
  while (*op_str)
    putChar2Buffer(buffer,*op_str++,size);
  for (size_t i = 0; i < n; ++i)
    putChar2Buffer(buffer,*operator_name++,size);
  input += 2;
  return true;
}

ssize_t Stabs2DebugInfo::getFunctionLine(pointer start, pointer offset) const
{
  ssize_t line = -1;
  ustl::map<size_t, StabEntry const *>::const_iterator it = function_symbols_.find(start);
  if (it != function_symbols_.end())
  {
    StabEntry const *se = it->second + 1;
    while (se->n_type == N_PSYM)
      ++se;
    while (se->n_type == N_SLINE)
    {
      if (offset < se->n_value)
        return line;
      else
        line = se->n_desc;
      ++se;
    }
  }
  return line;
}

pointer Stabs2DebugInfo::getFunctionName(pointer address, char function_name[], size_t size) const
{
  if (function_symbols_.size() == 0)
    return 0;

  ustl::map<size_t, StabEntry const *>::const_iterator it = function_symbols_.end();

  if (ADDRESS_BETWEEN(address, function_symbols_.begin()->first, ArchCommon::getKernelEndAddress()))
  {
    it = function_symbols_.begin();

    while (it != function_symbols_.end() && it->first <= address)
      ++it;
  }

  if (it != function_symbols_.end())
  {
    --it;
    demangleName(stabstr_buffer_ + it->second->n_strx, function_name, size);
    return it->first;
  }

  return 0;
}

int Stabs2DebugInfo::readNumber(const char *& input) const
{
  if (!input)
    return -1;

  int result = 0;

  while (*input >= '0' && *input <= '9')
  {
    result = 10 * result + (*input - '0');
    ++input;
  }

  return result;
}

struct StabsTypename
{
  const char mangled;
  const char* demangled;
} typenames[] = {
  {'v',""}, {'w',"wchar_t"}, {'b',"bool"}, {'c',"char"}, {'a',"signed char"}, {'h',"unsigned char"}, {'s',"short"},
  {'t',"unsigned short"}, {'i',"int"}, {'j',"unsigned int"}, {'l',"long"}, {'m',"unsigned long"}, {'x',"long long"},
  {'y',"unsigned long long"}, {'n',"__int128"}, {'o',"unsigned __int128"}, {'f',"float"}, {'d',"double"},
  {'e',"long double"}, {'g',"__float128"}, {'z',"ellipsis"}
};

void Stabs2DebugInfo::pasteTypename(const char *& input, char *& buffer, size_t& size) const
{
  bool is_reference = false, is_pointer = false;
  size_t offset_input, count;
  char const* src = "<?>";

  if (!input || !buffer)
    return;

  if (*input == 'R')
  {
    ++input;
    is_reference = true;
  }

  if (*input == 'P')
  {
    ++input;
    is_pointer = true;
  }

  if (*input == 'K')
  {
    const char* const_str = "const";
    while (*const_str)
      putChar2Buffer(buffer,*const_str++,size);
    ++input;
  }

  if (*input >= '0' && *input <= '9')
  {
    count = readNumber(input);
    offset_input = count;
    src = input;
  }
  else
  {
    src = "<?>";
    for (StabsTypename t : typenames)
    {
      if (*input == t.mangled)
      {
        src = t.demangled;
        break;
      }
    }
    count = strlen(src);
    offset_input = 1;
  }

  for (size_t i = 0; i < count; ++i)
    putChar2Buffer(buffer,*src++,size);
  input += offset_input;

  if (is_pointer)
    putChar2Buffer(buffer,'*',size);

  if (is_reference)
    putChar2Buffer(buffer,'&',size);
}

void Stabs2DebugInfo::pasteArguments(const char *& input, char *& buffer, char delimiter, size_t& size) const
{
  if (!input || !buffer)
    return;

  int arg_nr = 0;

  while (*input != delimiter)
  {
    if (arg_nr++)
    {
      putChar2Buffer(buffer,',',size);
      putChar2Buffer(buffer,' ',size);
    }

    pasteTypename(input, buffer, size);
  }
}

size_t Stabs2DebugInfo::putChar2Buffer(char*& buffer, char c, size_t& size) const
{
  if (size <= 1)
    return -1;
  *buffer++ = c;
  *buffer = 0;
  --size;
  return 0;
}

void Stabs2DebugInfo::demangleName(const char* name, char *buffer, size_t size) const
{
  const char *p_data = name;

  if (!buffer || !p_data)
    return;

  if (p_data[0] != '_' || p_data[1] != 'Z') // is the name mangled?
  {
    size_t length = strlen(name);
    for (size_t i = 0; i < length; ++i) // copy unmangled name
    {
      if (*p_data == ':')
        break;

      putChar2Buffer(buffer,*p_data++,size);
    }
  }
  else
  {
    p_data += 2; // skip "_Z", which indicates a mangled name

    if (*p_data == 'N') // we've got a nested name
    {
      ++p_data;
      int name_count = 0;

      uint32 repeat = 0;
      while (*p_data != 'E')
      {
        if (name_count++ && *p_data != 'I')
        {
          putChar2Buffer(buffer,':',size);
          putChar2Buffer(buffer,':',size);
        }

        if (*p_data >= '0' && *p_data <= '9') // part of nested name
        {
          size_t Count = readNumber(p_data);

          for (size_t i = 0; i < Count; ++i)
            putChar2Buffer(buffer,*p_data++,size);
        }
        else if (*p_data == 'I') // parse template params
        {
          putChar2Buffer(buffer,'<',size);
          pasteArguments(++p_data, buffer, 'E', size);
          putChar2Buffer(buffer,'>',size);
          ++p_data;
        }
        else
          tryPasteOoperator(p_data, buffer, size);
        if (repeat >= 2)
          break;
        ++repeat;
      }
      ++p_data;
    }
    else
    {
      if (*p_data == 'L') // we've got a local name
        ++p_data;

      size_t count = readNumber(p_data);
      for (size_t i = 0; i < count; ++i)
        putChar2Buffer(buffer,*p_data++,size);
    }

    putChar2Buffer(buffer,'(',size);
    pasteArguments(p_data, buffer, ':', size);
    putChar2Buffer(buffer,')',size);
  }
}

