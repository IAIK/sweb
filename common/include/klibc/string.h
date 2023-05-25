#pragma once

#include "stddef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    char *strcpy(char* dest, const char* src);
    char *strncpy(char* dest, const char* src, size_t size);
    char *strcat(char* dest, const char* append);
    char *strncat(char* dest, const char* append, size_t size);
    // size_t strxfrm( char* dest, const char* src, size_t count );
    size_t strlen(const char* str);
    int strcmp(const char* str1, const char* str2);
    int strncmp(const char* str1, const char* str2, size_t n);
    // int strcoll( const char* lhs, const char* rhs );
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wbuiltin-declaration-mismatch"
    char* strchr(const char* str, char c);
    char* strrchr(const char* str, char c);
#pragma GCC diagnostic pop
    // size_t strspn( const char* dest, const char* src );
    // size_t strcspn( const char *dest, const char *src );
    // const char* strpbrk( const char* dest, const char* breakset );
    // const char* strstr( const char* haystack, const char* needle );
    char* strtok(char* str, const char* delimiters);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wbuiltin-declaration-mismatch"
    void* memchr(const void* block, char c, size_t size);
#pragma GCC diagnostic pop

    int memcmp(const void* region1, const void* region2, size_t size);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wbuiltin-declaration-mismatch"
    void* memset(void* block, char c, size_t size);
#pragma GCC diagnostic pop

    void* memcpy(void* dest, const void* src, size_t length);
    void* memmove(void* dest, const void* src, size_t length);


#ifdef __cplusplus
}
#endif
