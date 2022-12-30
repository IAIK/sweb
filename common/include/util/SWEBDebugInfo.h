#pragma once

#include "EASTL/string.h"
#include "EASTL/map.h"
#include "EASTL/vector_map.h"
#include "Stabs2DebugInfo.h"


// The limit for function names, after that, they will get capped
#define CALL_FUNC_NAME_LIMIT 256
#define CALL_FUNC_NAME_LIMIT_STR macroToString(CALL_FUNC_NAME_LIMIT)

class SWEBDebugInfo : public Stabs2DebugInfo {
public:

    SWEBDebugInfo(char const *sweb_begin, char const *sweb_end);

    ~SWEBDebugInfo() override = default;

    void getCallNameAndLine(pointer address, const char *&mangled_name, ssize_t &line) const override;

    void printCallInformation(pointer address) const override;

private:
    eastl::vector_map<size_t, eastl::string> file_addrs_;
    eastl::vector_map<size_t, const char*> function_defs_;

    void initialiseSymbolTable() override;

};
