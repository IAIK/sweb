/**
 * ElfFormat.h
 */

#include "types.h"
#include "kprintf.h"

class Elf
{
  public:
    static const uint32 EI_NIDENT = 16;

// Elf file types

    static const uint32 ET_NONE = 0;
    static const uint32 ET_REL = 1;
    static const uint32 ET_EXEC = 2;
    static const uint32 ET_DYN = 3;
    static const uint32 ET_CORE = 4;
    static const uint32 ET_LOPROC = 0xff00;
    static const uint32 ET_HIPROC = 0xffff;

// Elf machine types

    static const uint32 EM_NONE = 0;
    static const uint32 EM_M32 = 1;
    static const uint32 EM_SPARC = 2;
    static const uint32 EM_386 = 3;
    static const uint32 EM_68K = 4;
    static const uint32 EM_88K = 5;
    static const uint32 EM_860 = 7;
    static const uint32 EM_MIPS = 8;
    static const uint32 EM_AMD64 = 62;

// Elf version

    static const uint32 EV_NONE = 0;
    static const uint32 EV_CURRENT = 1;

// ELF Magic numbers
// An ELF file should have MAG0,MAG1,MAG2,MAG3
// set to 0x7f,'E','L','F' respectively
//

    static const uint32 EI_MAG0 = 0;
    static const uint32 EI_MAG1 = 1;
    static const uint32 EI_MAG2 = 2;
    static const uint32 EI_MAG3 = 3;
    static const uint32 EI_CLASS = 4;
    static const uint32 EI_DATA = 5;
    static const uint32 EI_VERSION = 6;
    static const uint32 EI_OSABI = 7;
    static const uint32 EI_ABIVERSION = 8;
    static const uint32 EI_PAD = 9;

// ELF CLASSES

    static const uint32 ELFCLASSNONE = 0;
    static const uint32 ELFCLASS32 = 1;
    static const uint32 ELFCLASS64 = 2;

// ELF DATAS

    static const uint32 ELFDATANONE = 0;
    static const uint32 ELFDATA2LSB = 1;
    static const uint32 ELFDATA2MSB = 2;

// ELF SECTION HEADERS

    static const uint32 SHN_UNDEF = 0;
    static const uint32 SHN_LORESERVE = 0xff00;
    static const uint32 SHN_LOPROC = 0xff00;
    static const uint32 SHN_HIPROC = 0xff1f;
    static const uint32 SHN_ABS = 0xfff1;
    static const uint32 SHN_COMMON = 0xfff2;
    static const uint32 SHN_HIRESERVE = 0xffff;

    static const uint32 SHT_NULL = 0;
    static const uint32 SHT_PROGBITS = 1;
    static const uint32 SHT_SYMTAB = 2;
    static const uint32 SHT_STRTAB = 3;
    static const uint32 SHT_RELA = 4;
    static const uint32 SHT_HASH = 5;
    static const uint32 SHT_DYNAMIC = 6;
    static const uint32 SHT_NOTE = 7;
    static const uint32 SHT_NOBITS = 8;
    static const uint32 SHT_REL = 9;
    static const uint32 SHT_SHLIB = 10;
    static const uint32 SHT_DYNSYM = 11;
    static const uint32 SHT_LOPROC = 0x70000000;
    static const uint32 SHT_HIPROC = 0x7fffffff;
    static const uint32 SHT_LOUSER = 0x80000000;
    static const uint32 SHT_HIUSER = 0xffffffff;

// ELF Relocation types

    static const uint32 R_386_NONE = 0;
    static const uint32 R_386_32 = 1;
    static const uint32 R_386_PC32 = 2;
    static const uint32 R_386_GOT32 = 3;
    static const uint32 R_386_PLT32 = 4;
    static const uint32 R_386_COPY = 5;
    static const uint32 R_386_GLOB_DAT = 6;
    static const uint32 R_386_JMP_SLOT = 7;
    static const uint32 R_386_RELATIVE = 8;
    static const uint32 R_386_GOTOFF = 9;
    static const uint32 R_386_GOTPC = 10;

// ELF PROGRAM SEGMENT TYPES

    static const uint32 PT_NULL = 0;
    static const uint32 PT_LOAD = 1;
    static const uint32 PT_DYNAMIC = 2;
    static const uint32 PT_INTERP = 3;
    static const uint32 PT_NOTE = 4;
    static const uint32 PT_SHLIB = 5;
    static const uint32 PT_PHDR = 6;
    static const uint32 PT_LOPROC = 0x70000000;
    static const uint32 PT_HIPROC = 0x7fffffff;

    static const uint32 EXECUTEABLE = 1;
    static const uint32 WRITEABLE = 2;
    static const uint32 READABLE = 4;

    typedef uint64 Elf64_Addr;
    typedef uint16 Elf64_Half;
    typedef uint64 Elf64_Off;
    typedef int32 Elf64_Sword;
    typedef uint32 Elf64_Word;
    typedef int64 Elf64_Sxword;
    typedef uint64 Elf64_Xword;

    struct sELF64_Ehdr
    {
        uint8 e_ident[EI_NIDENT];
        Elf64_Half e_type;
        Elf64_Half e_machine;
        Elf64_Word e_version;
        Elf64_Addr e_entry;
        Elf64_Off e_phoff;
        Elf64_Off e_shoff;
        Elf64_Word e_flags;
        Elf64_Half e_ehsize;
        Elf64_Half e_phentsize;
        Elf64_Half e_phnum;
        Elf64_Half e_shentsize;
        Elf64_Half e_shnum;
        Elf64_Half e_shstrndx;
    } __attribute__((__packed__));

    struct sELF64_Shdr
    {
        Elf64_Word sh_name;
        Elf64_Word sh_type;
        Elf64_Xword sh_flags;
        Elf64_Addr sh_addr;
        Elf64_Off sh_offset;
        Elf64_Xword sh_size;
        Elf64_Word sh_link;
        Elf64_Word sh_info;
        Elf64_Xword sh_addralign;
        Elf64_Xword sh_entsize;
    } __attribute__((__packed__)) ;

    typedef struct
    {
        Elf64_Word st_name;
        uint8 st_info;
        uint8 st_other;
        Elf64_Half st_shndx;
        Elf64_Addr st_value;
        Elf64_Xword st_size;
    } __attribute__((__packed__)) ELF64_Sym;

    typedef struct
    {
        Elf64_Addr r_offset;
        Elf64_Xword r_info;
    } __attribute__((__packed__)) ELF64_Rel;

    typedef struct
    {
        Elf64_Addr r_offset;
        Elf64_Xword r_info;
        Elf64_Sxword r_added;
    } __attribute__((__packed__)) ELF64_Rela;

    struct sELF64_Phdr
    {
        Elf64_Word p_type;
        Elf64_Word p_flags;
        Elf64_Off p_offset;
        Elf64_Addr p_vaddr;
        Elf64_Addr p_paddr;
        Elf64_Xword p_filesz;
        Elf64_Xword p_memsz;
        Elf64_Xword p_align;
    } __attribute__((__packed__));

    typedef struct sELF64_Phdr Phdr;
    typedef struct sELF64_Shdr Shdr;
    typedef struct sELF64_Ehdr Ehdr;

    static bool headerCorrect(Ehdr* hdr)
    {
      if (hdr->e_ident[Elf::EI_MAG0]  == 0x7f
          ||  hdr->e_ident[Elf::EI_MAG1]  == 'E'
          ||  hdr->e_ident[Elf::EI_MAG2]  == 'L'
          ||  hdr->e_ident[Elf::EI_MAG3]  == 'F'
          ||  hdr->e_ident[Elf::EI_CLASS] == Elf::ELFCLASS64
          ||  hdr->e_ident[Elf::EI_DATA]  == Elf::ELFDATA2LSB
          ||  hdr->e_type            == Elf::ET_EXEC
          ||  hdr->e_machine         == Elf::EM_AMD64
          ||  hdr->e_version         == Elf::EV_CURRENT)
      {
        return true;
      }
      return false;
    }


    static void printElfHeader ( Ehdr &hdr )
    {
      kprintfd ( "hdr addr: %x\n",&hdr );
    #define foobar(x) kprintfd("hdr " #x ": %x\n",hdr. x)
      foobar ( e_type );
      foobar ( e_machine );
      foobar ( e_version );
      foobar ( e_entry );
      foobar ( e_phoff );
      foobar ( e_shoff );
      foobar ( e_flags );
      foobar ( e_ehsize );
      foobar ( e_phentsize );
      foobar ( e_phnum );
      foobar ( e_shentsize );
      foobar ( e_shnum );
      foobar ( e_shstrndx );
    }

};
