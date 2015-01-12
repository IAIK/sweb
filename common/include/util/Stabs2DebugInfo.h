

#include "umap.h"

class StabEntry;
class Stabs2DebugInfo
{
public:

  Stabs2DebugInfo(char const *stab_begin, char const *stab_end, char const *stab_str);
  ~Stabs2DebugInfo();

  ssize_t getFunctionLine(pointer start, pointer offset) const;
  pointer getFunctionName(pointer address, char function_name[])  const;

  void printAllFunctions() const;

private:

  void initialiseSymbolTable();


  bool tryPasteOoperator(const char *& input, char *& buffer) const;
  int readNumber(const char *& input) const;
  void pasteTypename(const char *& input, char *& buffer) const;
  void pasteArguments(const char *& input, char *& buffer, char delimiter) const;
  void demangleName(const char* name, char *buffer) const;


  StabEntry const *stab_start_;
  StabEntry const *stab_end_;
  char const *stabstr_buffer_;

  ustl::map<size_t, StabEntry const*> function_symbols_;
};
