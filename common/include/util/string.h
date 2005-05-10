/// @file string.h libc style string and array manipulation funtions
/// @author David Riebenbauer <davrieb@sbox.tugraz.at>


#ifndef STRING_H__
#define STRING_H__

#define STRING_SAVE

#include "types.h"

#ifdef __cplusplus
extern "C"
{
#endif

/// Calculate the length of a string.
/// @param str is a NULL-terminated charachter string.
/// @returns the length of str.
size_t strlen(const char* str);

/// copy length bytes from src to dest, which may not overlap
/// @return is a pointer to dest
void *memcpy(void *dest, const void *src, size_t length);

/// copy length bytes from src to dest, even if they overlap
/// @return is a pointer to dest
void *memmove(void *dest, const void *src, size_t length);

/// copy length bytes from src to dest, but stop in case the character c occurs
/// @return is a pointer to the byte past, where c was copied or a NULL pointer.
void *memccpy(void *dest, const void *src, uint8 c, size_t length);

/// initializes size bytes of the memory pointed to by block, with the value of c
/// @return is the beginnig of block
void *memset(void *block, uint8 c, size_t size);

/// Copys the string src to the string dest up to and including the terminating '\0' char.
/// @return is the value of dest.
char *strcpy(char *dest, const char* src);

/// Copys size chars from the string src to the string dest up to and including the terminating
/// '\0' char. If size is smaller than the length of src no terminating '\0' is written. If
/// size is greater ten the length of src the rest is filled up with '\0' charachters.
///
/// @return is the value of dest.
char *strncpy(char *dest, const char* src, size_t size);

/// Copys at most size-1 characters from src to dest. If size is smaller than the length of src
/// only the number of characters in src will be copied. The resulting string is
/// NULL-terminated, unless size == 0, in which case nothing ius done.
///
/// @return is the size of src.
size_t strlcpy(char* dest, const char* src, size_t size);

/// Copys src into a newly allocated string.
///
/// @return a newly allocated copy of src or NULL if the allocation failed.
char *strdup(const char *src);

/// Appends append to the end to dest replacing the '\0' charachter marking the end of dest.
///
/// @return is the pointer dest.
char *strcat(char *dest, const char*append);

/// Appends size charachters of append to the end to dest replacing the '\0' charachter marking
/// the end of dest. A '\0' charachter is also appended to dest.
///
/// @return is the pointer dest.
char *strncat(char *dest, const char*append, size_t size);

/// Appends the '\0' terminated string append to the end of dest. It will append at most
/// size - strlen(dest) - 1 bytes. The result will be '\0'-terminated.
///
/// @return the length of src + the length of dest.
size_t strlcat(char *dest, const char*append, size_t size);

/// copy length bytes from src to dest, even if they overlap
void bcopy(void *src, void* dest, size_t length);

/// overwrite size bytes of block with 0
void bzero(void *block, size_t size);

/// Compare size bytes of region1 and region2
///
/// @return the differenc between the first differing bytes or zero on equality
int32 memcmp(const void *region1, const void *region2, size_t size);

/// Compares the strings str1 and str2.
///
/// @return is an value smaller than 0 if str1, 0 if the strings are equal and greater than 0
/// if str1 is greater.
int32 strcmp(const char *str1, const char *str2);

/// Compares n charachters of the strings str1 and str2.
///
/// @return is an value smaller than 0 if str1, 0 if the strings are equal and greater than 0
/// if str1 is greater.
int32 strncmp(const char *str1, const char *str2, size_t n);

/// Compare size bytes of region1 and region2
///
/// @return the difference between the first differing bytes or zero on equality
int32 bcmp(const void *region1, const void *region2, size_t size);

/// Finds the first occurence of the byte c in the first size bytes of block.
///
/// @return is a pointer to the first occurence of c, or a null pointer.
void *memchr(const void *block, uint8 c, size_t size);

/// Finds the first occurence of the byte c in the first size bytes of block, searching
/// backwards. That means it actually searches for the last occurence of c.
///
/// @return is a pointer to the last occurence of c, or a null pointer.
void *memrchr(const void *block, uint8 c, size_t size);

/// Finds the first occurence of c in the string str.
///
/// @return is a pointer to the first occurecnce of c, or NULL in case that nothing was found.
char *strchr(const char* str, char c);

/// Finds the first occurence of c in the string str, searching backwards from the back.
///
/// @return
char *strrchr(const char* str, char c);

#ifdef __cplusplus
}
#endif

#endif

