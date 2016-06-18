#include "kprintf.h"
#include "SWEBDebugInfo.h"
#include "Stabs2DebugInfo.h"
#include "ArchCommon.h"
#include "ArchMemory.h"

SWEBDebugInfo::SWEBDebugInfo(char const *sweb_start, char const *sweb_end) : Stabs2DebugInfo(sweb_start, sweb_end, 0) {
    initialiseSymbolTable();
}

SWEBDebugInfo::~SWEBDebugInfo() {
    ustl::map<size_t, const char *>::const_iterator f_it;
    for (f_it = function_files_.begin(); f_it != function_files_.end(); ++f_it) delete[] f_it->second;
}

char *SWEBDebugInfo::readFilename(char **pdata) {
    char *data = *pdata;
    char *fn = new char[data[0] + 1];
    memcpy(fn, data + 1, (size_t) data[0]);
    fn[(size_t) fn[0]] = 0;
    *pdata += data[0] + 1;
    return fn;
}

size_t SWEBDebugInfo::readEntries(char **pdata) {
    char *data = *pdata;
    size_t entries = *(uint32_t * )(data);
    *pdata += 4;
    return entries;
}

SWEBDebugEntry SWEBDebugInfo::readEntry(char **pdata) {
    char *data = *pdata;
    SWEBDebugEntry e;
    e.address = *(uint64_t * )(data);
    e.line = *(uint32_t * )(data + 8);
    *pdata += 12;
    return e;
}

void SWEBDebugInfo::initialiseSymbolTable() {
    function_symbols_.reserve(256);
    function_files_.reserve(256);

    size_t i;
    char *data = (char *) stab_start_;

    do {
        char *filename = readFilename(&data);
        size_t entries = readEntries(&data);

        for (i = 0; i < entries; i++) {
            SWEBDebugEntry entry = readEntry(&data);

            function_symbols_[entry.address] = (StabEntry *) (pointer)entry.line;
            if (i == 0) function_files_[entry.address] = filename;
        }
    } while (data < (char *) stab_end_);
    debug(USERTRACE, "found %zd sweb debug functions\n", function_symbols_.size());

}

void SWEBDebugInfo::getCallNameAndLine(pointer address, const char *&name, ssize_t &line) const {
    name = "UNKNOWN FUNCTION";
    line = 0;

    if (!this || function_symbols_.size() == 0 ||
        !(ADDRESS_BETWEEN(address, function_symbols_.front().first, function_symbols_.back().first)))
        return;

    ustl::map < size_t, StabEntry const * > ::const_reverse_iterator it;
    for (it = function_symbols_.rbegin(); it != function_symbols_.rend() && it->first > address; ++it);

    if (it == function_symbols_.rend())
        return;

    ustl::map<size_t, const char *>::const_reverse_iterator f_it;
    for (f_it = function_files_.rbegin(); f_it != function_files_.rend() && f_it->first > address; ++f_it);
    if(f_it != function_files_.rend()) {
        name = f_it->second;
    }

    line = (ssize_t) it->second;
}


void SWEBDebugInfo::printCallInformation(pointer address) const {
    const char *name;
    ssize_t line;
    getCallNameAndLine(address, name, line);
    if (line >= 0) {
        kprintfd("%10zx: %." CALL_FUNC_NAME_LIMIT_STR "s:%zu \n", address, name, line );
    }
    else {
        kprintfd("%10zx: %." CALL_FUNC_NAME_LIMIT_STR "s+%zx\n", address, name, -line);
    }
}
