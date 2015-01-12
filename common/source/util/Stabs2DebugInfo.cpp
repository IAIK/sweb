#include "kprintf.h"
#include "Stabs2DebugInfo.h"
#include "ArchCommon.h"

#define ADDRESS_BETWEEN(Value, LowerBound, UpperBound) \
  ((((void*)Value) >= ((void*)LowerBound)) && (((void*)Value) < ((void*)UpperBound)))

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

struct StabEntry
{
    uint32 n_strx;
    uint8 n_type;
    uint8 n_other;
    uint16 n_desc;
    uint32 n_value;
} __attribute__((packed));




Stabs2DebugInfo::Stabs2DebugInfo(char const *stab_start, char const *stab_end, char const *stab_str) :
  stab_start_ (reinterpret_cast<StabEntry const *>(stab_start)),
  stab_end_ (reinterpret_cast<StabEntry const *>(stab_end)),
  stabstr_buffer_(stab_str)
{
  initialiseSymbolTable();
}

Stabs2DebugInfo::~Stabs2DebugInfo()
{
  // we reinterpret casted this from normal character buffers
  // so make sure we don't execute any potential dtors, no ctors were called anyway
  operator delete[] (reinterpret_cast<void*>(const_cast<StabEntry *>(stab_start_)));
  operator delete[] (reinterpret_cast<void*>(const_cast<char *>(stabstr_buffer_)));
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
}

bool Stabs2DebugInfo::tryPasteOoperator(const char *& input, char *& buffer) const
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

  if (ADDRESS_BETWEEN(address, function_symbols_.at(0).first, ArchCommon::getKernelEndAddress()))
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
  if (!input) return -1;

  int Result = 0;

  while (*input >= '0' && *input <= '9')
  {
    Result = 10*Result + (*input - '0');
    ++input;
  }

  return Result;
}

void Stabs2DebugInfo::pasteTypename(const char *& input, char *& buffer) const
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
    Count = readNumber(input);
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

void Stabs2DebugInfo::pasteArguments(const char *& input, char *& buffer, char delimiter) const
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

    pasteTypename(input, buffer);
  }
}

void Stabs2DebugInfo::demangleName(const char* name, char *buffer) const
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

