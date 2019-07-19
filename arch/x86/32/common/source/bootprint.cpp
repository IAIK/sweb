#include "types.h"
#include "offsets.h"
#include "ArchCommon.h"
#include "debug_bochs.h"
#include "kstring.h"

uint8 fb_row = 0;
uint8 fb_col = 0;

char* getFBAddr(uint8 row, uint8 col)
{
        return (char*)ArchCommon::getFBPtr(0) + ((row*80 + col) * 2);
}

void setFBrow(uint8 row)
{
        *(uint8*)VIRTUAL_TO_PHYSICAL_BOOT(&fb_row) = row;
}
void setFBcol(uint8 col)
{
        *(uint8*)VIRTUAL_TO_PHYSICAL_BOOT(&fb_col) = col;
}

uint8 getFBrow()
{
        return *(uint8*)VIRTUAL_TO_PHYSICAL_BOOT(&fb_row);
}
uint8 getFBcol()
{
        return *(uint8*)VIRTUAL_TO_PHYSICAL_BOOT(&fb_col);
}

uint8 getNextFBrow()
{
        return (getFBrow() == 24 ? 0 : getFBrow() + 1);
}

void clearFB()
{
        memset(getFBAddr(0, 0), 0, 80 * 25 * 2);
        setFBrow(0);
        setFBcol(0);
}



void clearFBrow(uint8 row)
{
        memset(getFBAddr(row, 0), 0, 80 * 2);
}

void FBnewline()
{
        uint8 next_row = getNextFBrow();
        clearFBrow(next_row);
        setFBrow(next_row);
        setFBcol(0);
}

void kputc(const char c)
{
        //writeChar2Bochs('C');
        if(c == '\n')
        {
                FBnewline();
        }
        else
        {
                if(getFBcol() == 80)
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

void kputs(const char* string)
{
        while(*string != '\0')
        {
                kputc(*string);
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
        kputc(nibbleToASCII(nibble_h));
        kputc(nibbleToASCII(nibble_l));
}

void putHex32(uint32 v)
{
        for(uint8 i = 1; i <= sizeof(v); ++i)
        {
                putHex8(*((char*)&v + sizeof(v) - i));
        }
}
