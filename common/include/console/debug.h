#ifndef DEBUG_H___
#define DEBUG_H___

#define OUTPUT_ENABLED 0x80000000

//group minix
const size_t MINIX              = 0x00010000;// | OUTPUT_ENABLED;
const size_t M_STORAGE_MANAGER  = 0x00010001;// | OUTPUT_ENABLED;
const size_t M_INODE            = 0x00010002;// | OUTPUT_ENABLED;
const size_t M_SB               = 0x00010004;// | OUTPUT_ENABLED;
const size_t M_ZONE             = 0x00010008;// | OUTPUT_ENABLED;

//group Block Device
const size_t BD                 = 0x00020000;
const size_t BD_MANAGER         = 0x00020001;
const size_t BD_VIRT_DEVICE     = 0x00020002;

//group Console
const size_t CONSOLE            = 0x00040000;
const size_t KPRINTF            = 0x00040001;

//group kernel
const size_t KERNEL             = 0x00080000;
const size_t CONDITION          = 0x00080001;
const size_t LOADER             = 0x00080002;
const size_t SCHEDULER          = 0x00080004 | OUTPUT_ENABLED;
const size_t SYSCALL            = 0x00080008 | OUTPUT_ENABLED;
const size_t MAIN               = 0x00080010 | OUTPUT_ENABLED;
const size_t THREAD             = 0x00080020 | OUTPUT_ENABLED;
const size_t USERPROCESS        = 0x00080040 | OUTPUT_ENABLED;
const size_t MOUNTMINIX         = 0x00080080 | OUTPUT_ENABLED;
const size_t BACKTRACE          = 0x00080100 | OUTPUT_ENABLED;
const size_t USERTRACE          = 0x00080200;// | OUTPUT_ENABLED;
//group memory management
const size_t MM                 = 0x00100000;
const size_t PM                 = 0x00100001 | OUTPUT_ENABLED;
const size_t KMM                = 0x00100002; // | OUTPUT_ENABLED;;

//group driver
const size_t DRIVER             = 0x00200000;
const size_t ATA_DRIVER         = 0x00200001;
const size_t IDE_DRIVER         = 0x00200002;
const size_t MMC_DRIVER         = 0x00200004;
//group arch
const size_t ARCH               = 0x00400000;
const size_t A_BOOT             = 0x00400001 | OUTPUT_ENABLED;
const size_t A_COMMON           = 0x00400002;
const size_t A_MEMORY           = 0x00400004;
const size_t A_SERIALPORT       = 0x00400008;
const size_t A_KB_MANAGER       = 0x00400010;
const size_t A_INTERRUPTS       = 0x00400020;

//group file system
const size_t FS                 = 0x00800000;// | OUTPUT_ENABLED;
const size_t RAMFS              = 0x00800001;// | OUTPUT_ENABLED;
const size_t DENTRY             = 0x00800002;// | OUTPUT_ENABLED;
const size_t PATHWALKER         = 0x00800004;// | OUTPUT_ENABLED;
const size_t PSEUDOFS           = 0x00800008;// | OUTPUT_ENABLED;
const size_t VFSSYSCALL         = 0x00800010;// | OUTPUT_ENABLED;
const size_t VFS                = 0x00800020;// | OUTPUT_ENABLED;

#endif //DEBUG_H___

