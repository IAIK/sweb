

#include "umap.h"

class StabEntry;
class Stabs2DebugInfo
{
public:

  Stabs2DebugInfo(char const *stab_begin, char const *stab_end, char const *stab_str);
  ~Stabs2DebugInfo();

  ssize_t getFunctionLine(pointer start, pointer offset) const;
  pointer getFunctionName(pointer address, char function_name[], size_t size)  const;

  void printAllFunctions() const;

private:

  void initialiseSymbolTable();


  bool tryPasteOoperator(const char *& input, char *& buffer, size_t& size) const;
  int readNumber(const char *& input) const;
  void pasteTypename(const char *& input, char *& buffer, size_t& size) const;
  void pasteArguments(const char *& input, char *& buffer, char delimiter, size_t& size) const;
  void demangleName(const char* name, char *buffer, size_t size) const;
  size_t putChar2Buffer(char*& buffer, char c, size_t& size) const;


  StabEntry const *stab_start_;
  StabEntry const *stab_end_;
  char const *stabstr_buffer_;

  ustl::map<size_t, StabEntry const*> function_symbols_;
};
