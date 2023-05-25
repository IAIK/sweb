#pragma once

#include "types.h"

struct [[gnu::packed]] SegmentDescriptor
{
    uint16 limitL;
    uint16 baseLL;

    uint8 baseLM;
    uint8 typeL;
    uint8 limitH : 4;
    uint8 typeH  : 4;
    uint8 baseLH;

    uint32 baseH;
    uint32 reserved;
};

struct [[gnu::packed]] GDT
{
    SegmentDescriptor entries[7];
};

struct [[gnu::packed]] GDT32Ptr
{
    uint16 limit;
    uint32 addr;

    GDT32Ptr() = default;
    GDT32Ptr(uint16 limit, uint32 addr);
    explicit GDT32Ptr(GDT& gdt);
    void load();
};

struct [[gnu::packed]] GDT64Ptr
{
    uint16 limit;
    uint64 addr;

    GDT64Ptr() = default;
    GDT64Ptr(uint16 limit, uint64 addr);
    explicit GDT64Ptr(GDT& gdt);
    void load();
};

struct [[gnu::packed]] TSSSegmentDescriptor
{
    uint32 limitL          : 16;
    uint32 baseLL          : 16;

    uint32 baseLM          :  8;
    uint32 type            :  4;
    uint32 zero            :  1;
    uint32 dpl             :  2;
    uint32 present         :  1;
    uint32 limitH          :  4;
    uint32 avl_to_software :  1;
    uint32 ignored         :  2;
    uint32 granularity     :  1;
    uint32 baseLH          :  8;

    uint32 baseH;

    uint32 reserved;
};

void setTSSSegmentDescriptor(TSSSegmentDescriptor* descriptor, uint32 baseH, uint32 baseL, uint32 limit, uint8 dpl);

struct [[gnu::packed]] TSS
{
    uint32 reserved_0;

    union
    {
        uint64 rsp0;

        struct
        {
            uint32 rsp0_l;
            uint32 rsp0_h;
        };
    };

    uint32 rsp1_l;
    uint32 rsp1_h;
    uint32 rsp2_l;
    uint32 rsp2_h;
    uint32 reserved_1;
    uint32 reserved_2;

    union
    {
        uint64 ist0;

        struct
        {
            uint32 ist0_l;
            uint32 ist0_h;
        };
    };

    uint32 reserved_3[14];
    uint16 reserved_4;
    uint16 iobp;

    void setTaskStack(size_t stack_top);
};

void setFSBase(size_t fs_base);
void setGSBase(size_t fs_base);
size_t getFSBase();
size_t getGSBase();
size_t getGSKernelBase();
void* getSavedFSBase();
void restoreSavedFSBase();
void setSWAPGSKernelBase(size_t swapgs_base);
