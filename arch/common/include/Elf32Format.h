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

    typedef uint32 Elf32_Addr;
    typedef uint16 Elf32_Half;
    typedef uint32 Elf32_Off;
    typedef int32 Elf32_Sword;
    typedef uint32 Elf32_Word;

    struct sELF32_Ehdr
    {
        uint8 e_ident[EI_NIDENT];
        Elf32_Half e_type;
        Elf32_Half e_machine;
        Elf32_Word e_version;
        Elf32_Addr e_entry;
        Elf32_Off e_phoff;
        Elf32_Off e_shoff;
        Elf32_Word e_flags;
        Elf32_Half e_ehsize;
        Elf32_Half e_phentsize;
        Elf32_Half e_phnum;
        Elf32_Half e_shentsize;
        Elf32_Half e_shnum;
        Elf32_Half e_shstrndx;

    };

    typedef struct sELF32_Ehdr Ehdr;

    static void printElfHeader(Ehdr &hdr)
    {
      kprintfd("hdr addr: %x\n", &hdr);
#define foobar(x) kprintfd("hdr " #x ": %x\n",hdr. x)
      foobar(e_type);
      foobar(e_machine);
      foobar(e_version);
      foobar(e_entry);
      foobar(e_phoff);
      foobar(e_shoff);
      foobar(e_flags);
      foobar(e_ehsize);
      foobar(e_phentsize);
      foobar(e_phnum);
      foobar(e_shentsize);
      foobar(e_shnum);
      foobar(e_shstrndx);
    }

    struct sELF32_Shdr
    {
        Elf32_Word sh_name;
        Elf32_Word sh_type;
        Elf32_Word sh_flags;
        Elf32_Addr sh_addr;
        Elf32_Off sh_offset;
        Elf32_Word sh_size;
        Elf32_Word sh_link;
        Elf32_Word sh_info;
        Elf32_Word sh_addralign;
        Elf32_Word sh_entsize;
    };
    typedef struct sELF32_Shdr Shdr;

    typedef struct
    {
        Elf32_Word st_name;
        Elf32_Addr st_value;
        Elf32_Word st_size;
        uint8 st_info;
        uint8 st_other;
        Elf32_Half st_shndx;
    } ELF32_Sym;

    typedef struct
    {
        Elf32_Addr r_offset;
        Elf32_Word r_info;
    } ELF32_Rel;

    typedef struct
    {
        Elf32_Addr r_offset;
        Elf32_Word r_info;
        Elf32_Sword r_added;
    } ELF32_Rela;

    struct sELF32_Phdr
    {
        Elf32_Word p_type;
        Elf32_Off p_offset;
        Elf32_Addr p_vaddr;
        Elf32_Addr p_paddr;
        Elf32_Word p_filesz;
        Elf32_Word p_memsz;
        Elf32_Word p_flags;
        Elf32_Word p_align;
    };

    typedef struct sELF32_Phdr Phdr;


    static bool headerCorrect(Ehdr* hdr)
    {
      if (hdr->e_ident[Elf::EI_MAG0]  == 0x7f
          ||  hdr->e_ident[Elf::EI_MAG1]  == 'E'
          ||  hdr->e_ident[Elf::EI_MAG2]  == 'L'
          ||  hdr->e_ident[Elf::EI_MAG3]  == 'F'
          ||  hdr->e_ident[Elf::EI_CLASS] == Elf::ELFCLASS32
          ||  hdr->e_ident[Elf::EI_DATA]  == Elf::ELFDATA2LSB
          ||  hdr->e_type            == Elf::ET_EXEC
          ||  hdr->e_machine         == Elf::EM_386
          ||  hdr->e_version         == Elf::EV_CURRENT)
      {
        return true;
      }
      return false;
    }
};
