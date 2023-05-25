#pragma once

#include <cinttypes>

class BDDriver;
class BDVirtualDevice;

struct MasterBootRecord
{
    struct PartitionEntry
    {
        enum class BootableStatus : uint8_t
        {
            NOT_BOOTABLE = 0,
            BOOTABLE = 0x80,
        };

        struct CHSAddress
        {
            uint8_t head;
            uint8_t sect;
            uint8_t cyl;
        };

        BootableStatus bootable;
        CHSAddress first_sector;
        uint8_t type;
        CHSAddress last_sector;
        uint32_t first_sector_lba;
        uint32_t num_sectors;
    } __attribute__((packed));

    static_assert(sizeof(PartitionEntry) == 16);

    static constexpr uint16_t PC_MBR_SIGNATURE = 0xAA55;

    uint8_t bootinst[446]; // GRUB space
    PartitionEntry parts[4];
    uint16_t signature; // set to 0xAA55 for PC MBR
} __attribute__((packed));

static_assert(sizeof(MasterBootRecord) == 512);

/*
    Partition types (taken from fdisk)

    00 Empty            27 Hidden NTFS Win  82 Linux swap / So  c1 DRDOS/sec (FAT-
    01 FAT12            39 Plan 9           83 Linux            c4 DRDOS/sec (FAT-
    02 XENIX root       3c PartitionMagic   84 OS/2 hidden or   c6 DRDOS/sec (FAT-
    03 XENIX usr        40 Venix 80286      85 Linux extended   c7 Syrinx
    04 FAT16 <32M       41 PPC PReP Boot    86 NTFS volume set  da Non-FS data
    05 Extended         42 SFS              87 NTFS volume set  db CP/M / CTOS / .
    06 FAT16            4d QNX4.x           88 Linux plaintext  de Dell Utility
    07 HPFS/NTFS/exFAT  4e QNX4.x 2nd part  8e Linux LVM        df BootIt
    08 AIX              4f QNX4.x 3rd part  93 Amoeba           e1 DOS access
    09 AIX bootable     50 OnTrack DM       94 Amoeba BBT       e3 DOS R/O
    0a OS/2 Boot Manag  51 OnTrack DM6 Aux  9f BSD/OS           e4 SpeedStor
    0b W95 FAT32        52 CP/M             a0 IBM Thinkpad hi  ea Linux extended
    0c W95 FAT32 (LBA)  53 OnTrack DM6 Aux  a5 FreeBSD          eb BeOS fs
    0e W95 FAT16 (LBA)  54 OnTrackDM6       a6 OpenBSD          ee GPT
    0f W95 Ext'd (LBA)  55 EZ-Drive         a7 NeXTSTEP         ef EFI (FAT-12/16/
    10 OPUS             56 Golden Bow       a8 Darwin UFS       f0 Linux/PA-RISC b
    11 Hidden FAT12     5c Priam Edisk      a9 NetBSD           f1 SpeedStor
    12 Compaq diagnost  61 SpeedStor        ab Darwin boot      f4 SpeedStor
    14 Hidden FAT16 <3  63 GNU HURD or Sys  af HFS / HFS+       f2 DOS secondary
    16 Hidden FAT16     64 Novell Netware   b7 BSDI fs          f8 EBBR protective
    17 Hidden HPFS/NTF  65 Novell Netware   b8 BSDI swap        fb VMware VMFS
    18 AST SmartSleep   70 DiskSecure Mult  bb Boot Wizard hid  fc VMware VMKCORE
    1b Hidden W95 FAT3  75 PC/IX            bc Acronis FAT32 L  fd Linux raid auto
    1c Hidden W95 FAT3  80 Old Minix        be Solaris boot     fe LANstep
    1e Hidden W95 FAT1  81 Minix / old Lin  bf Solaris          ff BBT
*/

enum PartitionType
{
    EMPTY = 0x00,
    FAT16 = 0x04,
    DOS_EXTENDED_CHS = 0x05,
    NTFS = 0x07,
    FAT32_CHS = 0x0B,
    FAT32_LBA = 0x0C,
    FAT16B_LBA = 0x0E,
    WINDOWS_EXTENDED_LBA = 0x0F,
    PLAN9 = 0x39,
    MINIXFS_ALT = 0x41, // Same as 0x81
    SWAP_ALT = 0x42, // Same as 0x82
    MINIXFS_OLD = 0x80,
    MINIXFS = 0x81,
    SWAP = 0x82,
    LINUX_ANY_NATIVE = 0x83,
    LINUX_EXTENDED = 0x85,
    LINUX_LVM = 0x8E,
    HFS = 0xAF,
    LINUX_LUKS = 0xE8,
    GPT_PROTECTIVE_MBR = 0xEE,
    EFI_SYSTEM = 0xEF,
    LINUX_LVM_OLD = 0xFE,
};

constexpr const char* partitionTypeName(uint8_t type)
{
    switch (type)
    {
    case EMPTY:
        return "empty";
    case FAT16:
        return "FAT16";
    case DOS_EXTENDED_CHS:
        return "DOS Extended";
    case NTFS:
        return "NTFS";
    case FAT32_CHS:
        return "FAT32 (CHS)";
    case FAT32_LBA:
        return "FAT32 (LBA)";
    case FAT16B_LBA:
        return "FAT16B (LBA)";
    case WINDOWS_EXTENDED_LBA:
        return "Windows Extended (LBA)";
    case PLAN9:
        return "Plan9";
    case MINIXFS_ALT:
        return "Minixfs (alternative)";
    case SWAP_ALT:
        return "Swap (alternative)";
    case MINIXFS_OLD:
        return "Minix FS (old)";
    case MINIXFS:
        return "Minix FS";
    case SWAP:
        return "Swap";
    case LINUX_ANY_NATIVE:
        return "Linux (any native FS)";
    case LINUX_EXTENDED:
        return "Linux Extended";
    case LINUX_LVM:
        return "Linux LVM";
    case HFS:
        return "HFS/HFS+";
    case LINUX_LUKS:
        return "Linux LUKS";
    case GPT_PROTECTIVE_MBR:
        return "GPT Protective MBR";
    case EFI_SYSTEM:
        return "EFI System";
    case LINUX_LVM_OLD:
        return "Linux LVM (old)";
    default:
        return "unknown";
    }
}


int detectMBRPartitions(BDVirtualDevice* bdv, BDDriver* drv, uint32_t sector, uint32_t SPT, const char* name);
