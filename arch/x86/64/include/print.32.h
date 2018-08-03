#pragma once
#include "types.h"

void memset(char* block, char c, size_t length);

void setFBrow(uint8 row);
void setFBcol(uint8 col);

uint8 getFBrow();
uint8 getFBcol();

uint8 getNextFBrow();

void clearFB();

char* getFBAddr(uint8 row, uint8 col);

void clearFBrow(uint8 row);

void FBnewline();

void putc(const char c);

void puts(const char* string);

uint8 nibbleToASCII(char nibble);

void putHex8(char c);

void putHex32(uint32 v);
