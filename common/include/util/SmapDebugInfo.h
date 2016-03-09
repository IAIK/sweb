#pragma once

#include "umap.h"
#include "Stabs2DebugInfo.h"


// The limit for function names, after that, they will get capped
#define CALL_FUNC_NAME_LIMIT 256
#define CALL_FUNC_NAME_LIMIT_STR macroToString(CALL_FUNC_NAME_LIMIT)

class SmapDebugInfo : public Stabs2DebugInfo
{
public:

  SmapDebugInfo(char const *smap_begin, char const *smap_end);
  ~SmapDebugInfo();

  virtual void getCallNameAndLine(pointer address, const char*& mangled_name, ssize_t &line) const;
  virtual void printCallInformation(pointer address) const;

private:

  virtual void initialiseSymbolTable();

};
