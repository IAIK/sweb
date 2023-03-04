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
