#pragma once

#include <cstdint>

extern "C" int64_t __divdi3(int64_t num, int64_t den);
extern "C" uint64_t __udivdi3(uint64_t num, uint64_t den);
extern "C" uint64_t __umoddi3(uint64_t a, uint64_t b);
extern "C" uint64_t __udivmoddi4(uint64_t num, uint64_t den, uint64_t *rem_p);

