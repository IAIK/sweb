#pragma once

#include "arch_backtrace.h"

#include "EASTL/vector_map.h"

// The limit for function names, after that, they will get capped
#define CALL_FUNC_NAME_LIMIT 256
#define CALL_FUNC_NAME_LIMIT_STR macroToString(CALL_FUNC_NAME_LIMIT)

struct StabEntry;
class Stabs2DebugInfo
{
public:
    Stabs2DebugInfo(const char* stab_begin, const char* stab_end, const char* stab_str);
    virtual ~Stabs2DebugInfo();

    virtual void
    getCallNameAndLine(pointer address, const char*& mangled_name, ssize_t& line) const;
    virtual void printCallInformation(pointer address) const;

protected:
    const StabEntry* stab_start_;
    const StabEntry* stab_end_;
    const char* stabstr_buffer_;

    eastl::vector_map<size_t, const StabEntry*> function_symbols_;

private:
  virtual void initialiseSymbolTable();

  virtual pointer getFunctionName(pointer address, char function_name[], size_t size)  const;
  ssize_t getFunctionLine(pointer start, pointer offset) const;
  bool tryPasteOoperator(const char *& input, char *& buffer, size_t& size) const;
  static int readNumber(const char *& input);
  void pasteTypename(const char *& input, char *& buffer, size_t& size) const;
  void pasteArguments(const char *& input, char *& buffer, char delimiter, size_t& size) const;
  void demangleName(const char* name, char *buffer, size_t size) const;
  static size_t putChar2Buffer(char*& buffer, char c, size_t& size);
};
