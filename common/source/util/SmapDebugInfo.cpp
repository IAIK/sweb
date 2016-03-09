#include "kprintf.h"
#include "SmapDebugInfo.h"
#include "Stabs2DebugInfo.h"
#include "ArchCommon.h"
#include "ArchMemory.h"

SmapDebugInfo::SmapDebugInfo(char const *smap_start, char const *smap_end) : Stabs2DebugInfo(smap_start, smap_end,0)
{
  initialiseSymbolTable();
}

SmapDebugInfo::~SmapDebugInfo()
{
}

void SmapDebugInfo::initialiseSymbolTable()
{
  size_t fail = 0;
  size_t addr = 0;
  char* p = (char*) stab_start_;
  while (!fail && p < (char*)stab_end_)
  {
    if (memcmp(p," 0x",3) == 0)
      p += 3;
    else
      fail = 1;
    addr = 0;
    for (size_t i = 0; i < 16; ++i)
    {
      addr <<= 4;
      if (*p >= 'a' && *p <= 'f')
        addr |= *p - 'a' + 10;
      else if (*p >= '0' && *p <= '9')
        addr |= *p - '0';
      else
        fail = 1;
      p++;
    }
    if (memcmp(p," ",1) == 0)
      p += 1;
    else
      fail = 1;
    StabEntry* pstart = (StabEntry*)p;
    while (*p != '\n')
      p += 1;
    *p = 0;
    function_symbols_[addr] = pstart;
    p++;
  }
  debug(MAIN, "found %zd smap functions\n", function_symbols_.size());

}

void SmapDebugInfo::getCallNameAndLine(pointer address, const char*& name, ssize_t &line) const
{
  name = "UNKNOWN FUNCTION";
  line = 0;

  if (!this || function_symbols_.size() == 0 ||
      !(ADDRESS_BETWEEN(address, function_symbols_.front().first, function_symbols_.back().first)))
    return;

  ustl::map<size_t, StabEntry const *>::const_iterator it;
  for(it = function_symbols_.begin(); it != function_symbols_.end() && it->first <= address; ++it);

  if (it == function_symbols_.end())
    return;

  --it;
  name = (const char*) it->second;
  line = it->first - address;
}



void SmapDebugInfo::printCallInformation(pointer address) const
{
  const char* name;
  ssize_t line;
  getCallNameAndLine(address, name, line);
  if(line >= 0)
  {
    kprintfd("%10zx: %." CALL_FUNC_NAME_LIMIT_STR "s:%zu \n", address, name, line );
  }
  else
  {
    kprintfd("%10zx: %." CALL_FUNC_NAME_LIMIT_STR "s+%zx\n", address, name, -line);
  }
}
