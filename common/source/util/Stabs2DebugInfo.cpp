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
  char *buffer = new char[1000];
  debug(MAIN, "Known symbols:\n");
  for (auto symbol : function_symbols_)
  {
    demangleName(stabstr_buffer_ + symbol.second->n_strx, buffer);
    debug(MAIN, "\t%s\n", buffer);
  }
  delete[] buffer;
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

bool Stabs2DebugInfo::tryPasteOoperator(const char *& input, char *& buffer) const
{
  if (!input || !buffer)
    return false;

  const char* operatorName = 0;

  for (StabsOperator op : operators)
  {
    if (input[0] == op.mangled[0] && input[1] == op.mangled[1])
    {
      operatorName = op.demangled;
      break;
    }
  }

  if (operatorName == 0)
    return false;

  int n = strlen(operatorName);

  memcpy(buffer, "operator", 8);
  buffer += 8;
  memcpy(buffer, operatorName, n);
  buffer += n;
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

pointer Stabs2DebugInfo::getFunctionName(pointer address, char function_name[]) const
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
    demangleName(stabstr_buffer_ + it->second->n_strx, function_name);
    return it->first;
  }

  return 0;
}

int Stabs2DebugInfo::readNumber(const char *& input) const
{
  if (!input)
    return -1;

  int Result = 0;

  while (*input >= '0' && *input <= '9')
  {
    Result = 10 * Result + (*input - '0');
    ++input;
  }

  return Result;
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

void Stabs2DebugInfo::pasteTypename(const char *& input, char *& buffer) const
{
  bool isReference = false, isPointer = false;
  int offsetInput, count;
  char const* src = "<?>";

  if (!input || !buffer)
    return;

  if (*input == 'R')
  {
    ++input;
    isReference = true;
  }

  if (*input == 'P')
  {
    ++input;
    isPointer = true;
  }

  if (*input == 'K')
  {
    memcpy(buffer, "const ", 6);
    ++input;
    buffer += 6;
  }

  if (*input >= '0' && *input <= '9')
  {
    count = readNumber(input);
    offsetInput = count;
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
    offsetInput = 1;
  }

  memcpy(buffer, src, count);
  buffer += count;
  input += offsetInput;

  if (isPointer)
    *buffer++ = '*';

  if (isReference)
    *buffer++ = '&';
}

void Stabs2DebugInfo::pasteArguments(const char *& input, char *& buffer, char delimiter) const
{
  if (!input || !buffer)
    return;

  int ArgNr = 0;

  while (*input != delimiter)
  {
    if (ArgNr++)
    {
      *buffer++ = ',';
      *buffer++ = ' ';
    }

    pasteTypename(input, buffer);
  }
}

void Stabs2DebugInfo::demangleName(const char* name, char *buffer) const
{
  const char *pData = name;

  if (!buffer || !pData)
    return;

  if (pData[0] != '_' || pData[1] != 'Z') // is the name mangled?
  {
    size_t length = strlen(name);
    for (size_t i = 0; i < length; ++i) // copy unmangled name
    {
      if (*pData == ':')
        break;

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
        int Count = readNumber(pData);

        memcpy(buffer, pData, Count);
        pData += Count;
        buffer += Count;
      }
      else if (*pData == 'I') // parse template params
      {
        *buffer++ = '<';
        pasteArguments(++pData, buffer, 'E');
        *buffer++ = '>';
        ++pData;
      }
      else
        tryPasteOoperator(pData, buffer);
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

    int Count = readNumber(pData);
    memcpy(buffer, pData, Count);
    buffer += Count;
    pData += Count;
  }

  *buffer++ = '(';
  pasteArguments(pData, buffer, ':');
  *buffer++ = ')';
  *buffer = '\0';
}

