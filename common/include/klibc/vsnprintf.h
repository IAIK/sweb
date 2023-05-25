#pragma once

#include "stdarg.h"
#include "stddef.h"

#define MAXNBUF 40

#define hex2ascii(hex)  (hex2ascii_data[hex])
#define hex2asciiupper(hex)  (hex2ascii_data_upper[hex])
const char hex2ascii_data[] = "0123456789abcdefghijklmnopqrstuvwxyz";
const char hex2ascii_data_upper[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned long u_long;
typedef unsigned int u_int;

struct snprintf_arg {
        char    *str;
        size_t  remain;
};

extern "C" int
vsnprintf(char *str, size_t size, const char *format, va_list ap);

extern void
snprintf_func(int ch, void *arg);

extern int
kvprintf(const char* fmt, void (*func)(int, void*), void* arg, int radix, va_list ap);

extern char *
ksprintn(char* nbuf, u_long ul, int base, int* lenp, int upper);

int snprintf(char* buf, size_t bufsz, const char* format, ...) __attribute__((format(printf, 3, 4)));
