asm(".code32");
#include "types.h"
#include "offsets.h"

#define FB_ADDR ((char*)0xB8000)
#define FB_COLS 80
#define FB_ROWS 25

uint8 fb_row = 0;
uint8 fb_col = 0;

void memset(char* block, char c, size_t length)
{
        for (size_t i = 0; i < length; ++i)
        {
                block[i] = c;
        }
}

void setFBrow(uint8 row)
{
        *(uint8*)TRUNCATE(&fb_row) = row;
}
void setFBcol(uint8 col)
{
        *(uint8*)TRUNCATE(&fb_col) = col;
}

uint8 getFBrow()
{
        return *(uint8*)TRUNCATE(&fb_row);
}
uint8 getFBcol()
{
        return *(uint8*)TRUNCATE(&fb_col);
}

uint8 getNextFBrow()
{
        return (getFBrow() == FB_ROWS-1 ? 0 : getFBrow() + 1);
}

void clearFB()
{
        memset(FB_ADDR, 0, FB_COLS * FB_ROWS * 2);
        setFBrow(0);
        setFBcol(0);
}

char* getFBAddr(uint8 row, uint8 col)
{
        return FB_ADDR + ((row*FB_COLS + col) * 2);
}

void clearFBrow(uint8 row)
{
        memset(getFBAddr(row, 0), 0, FB_COLS * 2);
}

void FBnewline()
{
        uint8 next_row = getNextFBrow();
        clearFBrow(next_row);
        setFBrow(next_row);
        setFBcol(0);
}

void putc(const char c)
{
        if(c == '\n')
        {
                FBnewline();
        }
        else
        {
                if(getFBcol() == FB_COLS)
                {
                        FBnewline();
                }

                uint32 row = getFBrow();
                uint32 col = getFBcol();

                char* fb_pos = getFBAddr(row, col);
                fb_pos[0] = c;
                fb_pos[1] = 0x02;

                setFBcol(getFBcol() + 1);
        }
}

void puts(const char* string)
{
        while(*string != '\0')
        {
                putc(*string);
                ++string;
        }
}

uint8 nibbleToASCII(char nibble)
{
        nibble &= 0xF;
        return nibble < 10 ? '0' + nibble :
                             'A' + nibble - 10;
}

void putHex8(char c)
{
        char nibble_l = c & 0xF;
        char nibble_h = c >> 4;
        putc(nibbleToASCII(nibble_h));
        putc(nibbleToASCII(nibble_l));
}

void putHex32(uint32 v)
{
        for(uint8 i = 1; i <= sizeof(v); ++i)
        {
                putHex8(*((char*)&v + sizeof(v) - i));
        }
}


asm(".code64");
