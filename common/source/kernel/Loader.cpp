//----------------------------------------------------------------------
//   $Id: Loader.cpp,v 1.8 2005/07/12 17:53:13 btittelbach Exp $
//----------------------------------------------------------------------
//
//  $Log: Loader.cpp,v $
//  Revision 1.7  2005/07/06 13:29:37  btittelbach
//  testing
//
//  Revision 1.6  2005/07/05 20:22:56  btittelbach
//  some changes
//
//  Revision 1.5  2005/07/05 17:29:48  btittelbach
//  new kprintf(d) Policy:
//  [Class::]Function: before start of debug message
//  Function can be abbreviated "ctor" if Constructor
//  use kprintfd where possible
//
//  Revision 1.4  2005/06/14 18:22:37  btittelbach
//  RaceCondition anfälliges LoadOnDemand implementiert,
//  sollte optimalerweise nicht im InterruptKontext laufen
//
//  Revision 1.3  2005/06/14 13:54:55  nomenquis
//  foobarpratz
//
//  Revision 1.2  2005/06/14 12:55:21  nomenquis
//  foobar
//
//  Revision 1.1  2005/05/31 18:25:49  nomenquis
//  forgot to add loader
//
//----------------------------------------------------------------------

#include "Loader.h"
#include "console/kprintf.h"
#include "ArchThreads.h"
#include "mm/PageManager.h"
#include "ArchMemory.h"
#define EI_NIDENT	16

// Elf file types

#define ET_NONE			0     
#define ET_REL			1     
#define ET_EXEC			2
#define ET_DYN			3
#define ET_CORE			4
#define ET_LOPROC		0xff00
#define ET_HIPROC		0xffff

// Elf machine types

#define EM_NONE			0
#define EM_M32			1
#define EM_SPARC		2
#define EM_386			3
#define EM_68K			4
#define EM_88K			5
#define EM_860			7
#define EM_MIPS			8

// Elf version

#define EV_NONE			0
#define EV_CURRENT		1

// -----------------------------    ELF Magic numbers
//  An ELF file should have MAG0,MAG1,MAG2,MAG3
//  set to 0x7f,'E','L','F' respectively
//

#define EI_MAG0			0
#define EI_MAG1			1
#define EI_MAG2			2
#define EI_MAG3			3
#define EI_CLASS		4
#define EI_DATA			5
#define EI_VERSION		6
#define EI_PAD			7


  //                     ELF CLASSES ------------
  
#define ELFCLASSNONE		0
#define ELFCLASS32		1
#define ELFCLASS64		2

  //                     ELF DATAS ----------------------------

#define ELFDATANONE		0
#define ELFDATA2LSB		1
#define ELFDATA2MSB		2


// ------------------ ELF SECTION HEADERS

#define SHN_UNDEF		0
#define SHN_LORESERVE		0xff00
#define SHN_LOPROC		0xff00
#define SHN_HIPROC		0xff1f
#define SHN_ABS			0xfff1
#define SHN_COMMON		0xfff2
#define SHN_HIRESERVE		0xffff
  

#define SHT_NULL		0
#define SHT_PROGBITS		1
#define SHT_SYMTAB		2
#define SHT_STRTAB		3
#define SHT_RELA		4
#define SHT_HASH		5
#define SHT_DYNAMIC		6
#define SHT_NOTE 		7
#define SHT_NOBITS		8
#define SHT_REL			9
#define SHT_SHLIB		10
#define SHT_DYNSYM		11
#define SHT_LOPROC		0x70000000
#define SHT_HIPROC		0x7fffffff
#define SHT_LOUSER		0x80000000
#define SHT_HIUSER		0xffffffff


// ---------------- ELF Relocation types

#define R_386_NONE		0
#define R_386_32		1
#define R_386_PC32		2
#define R_386_GOT32		3
#define R_386_PLT32		4
#define R_386_COPY		5
#define R_386_GLOB_DAT		6
#define R_386_JMP_SLOT		7
#define R_386_RELATIVE		8
#define R_386_GOTOFF		9
#define R_386_GOTPC		10

// ----------------- ELF PROGRAM SEGMENT TYPES

#define PT_NULL			0
#define PT_LOAD			1
#define PT_DYNAMIC		2
#define PT_INTERP		3
#define PT_NOTE			4
#define PT_SHLIB		5
#define PT_PHDR	 		6
#define PT_LOPROC		0x70000000
#define PT_HIPROC		0x7fffffff

// thanks spoon for typing in all this crap

typedef uint32 Elf32_Addr;
typedef uint16 Elf32_Half;
typedef uint32 Elf32_Off;
typedef int32 Elf32_Sword;
typedef uint32 Elf32_Word;


typedef struct
{
   uint8	e_ident[EI_NIDENT];
   Elf32_Half		e_type;
   Elf32_Half		e_machine;
   Elf32_Word		e_version;
   Elf32_Addr		e_entry;
   Elf32_Off		e_phoff;
   Elf32_Off		e_shoff;
   Elf32_Word		e_flags;
   Elf32_Half		e_ehsize;
   Elf32_Half		e_phentsize;
   Elf32_Half		e_phnum;
   Elf32_Half		e_shentsize;
   Elf32_Half		e_shnum;
   Elf32_Half		e_shstrndx;

} ELF32_Ehdr;


typedef struct
{
   Elf32_Word		sh_name;
   Elf32_Word		sh_type;
   Elf32_Word		sh_flags;
   Elf32_Addr		sh_addr;
   Elf32_Off		sh_offset;
   Elf32_Word		sh_size;
   Elf32_Word		sh_link;
   Elf32_Word		sh_info;
   Elf32_Word		sh_addralign;
   Elf32_Word		sh_entsize;
} ELF32_Shdr;


typedef struct
{
   Elf32_Word		st_name;
   Elf32_Addr		st_value;
   Elf32_Word		st_size;
   uint8	st_info;
   uint8	st_other;
   Elf32_Half		st_shndx;
} ELF32_Sym;


typedef struct
{
   Elf32_Addr		r_offset;
   Elf32_Word		r_info;
} ELF32_Rel;

typedef struct
{
   Elf32_Addr		r_offset;
   Elf32_Word		r_info;
   Elf32_Sword		r_added;
} ELF32_Rela;

typedef struct
{
   Elf32_Word		p_type;
   Elf32_Off		p_offset;
   Elf32_Addr		p_vaddr;
   Elf32_Addr		p_paddr;
   Elf32_Word		p_filesz;
   Elf32_Word		p_memsz;
   Elf32_Word		p_flags;
   Elf32_Word		p_align;
} ELF32_Phdr;

Loader::Loader(uint8 *file_image, Thread *thread) : file_image_(file_image),
  thread_(thread)
{
}

void Loader::initUserspaceAddressSpace()
{
  page_dir_page_ = PageManager::instance()->getFreePhysicalPage();
  kprintfd("Loader::initUserspaceAddressSpace: Got new Page no. %d\n",page_dir_page_);
  ArchMemory::initNewPageDirectory(page_dir_page_);
  kprintfd("Loader::initUserspaceAddressSpace: Initialised the page dir\n");

  uint32 page_for_stack = PageManager::instance()->getFreePhysicalPage();

  ArchMemory::mapPage(page_dir_page_,2*1024*256-1, page_for_stack,1);
  
  
}


uint32 Loader::loadExecutableAndInitProcess()
{
  // first of all say hello
  //~ kprintfd("Loader::loadExecutableAndInitProcess: going to load an executable\n");
  
  initUserspaceAddressSpace();
  
  ELF32_Ehdr *hdr = reinterpret_cast<ELF32_Ehdr *>(file_image_);
  
  //~ kprintfd("Loader::loadExecutableAndInitProcess: %c%c%c%c%c\n",file_image_[0],file_image_[1],file_image_[2],file_image_[3],file_image_[4]);
  //~ kprintfd("Loader::loadExecutableAndInitProcess: Sizeof %d %d %d %d\n",sizeof(uint64),sizeof(uint32),sizeof(uint16),sizeof(uint8));
  //~ kprintfd("Loader::loadExecutableAndInitProcess: Num ents: %d\n",hdr->e_phnum);
  kprintfd("Loader::loadExecutableAndInitProcess: Entry: %x\n",hdr->e_entry);
  
  //~ uint32 i;
  //~ for (i=0;i<hdr->e_phnum;++i)
  //~ {
    //~ ELF32_Phdr *h = (ELF32_Phdr *)((uint32)file_image_ + hdr->e_phoff + i* hdr->e_phentsize);
    //~ kprintf("Loader: PHdr[%d].vaddr=%x .paddr=%x .type=%x .memsz=%x .filez=%x .poff=%x\n",i,h->p_vaddr,h->p_paddr,h->p_type,h->p_memsz,h->p_filesz,h->p_offset);
    
    //~ pointer ptr = h->p_paddr;
    //~ if (ptr % PAGE_SIZE)
      //~ kprintf("Loader: Hell broke loose\n");
    
    //~ uint32 page_free = 0;
    //~ uint32 still_to_write = h->p_memsz;
    //~ uint32 still_to_read = h->p_filesz;
    //~ uint8 *curr_ptr = 0;
    //~ uint32 read = 0;
    //~ while (still_to_write)
    //~ {
      //~ if (!page_free)
      //~ {
        //~ uint32 page = PageManager::instance()->getFreePhysicalPage();
        //~ ArchMemory::mapPage(page_dir_page_,ptr/PAGE_SIZE,page,1);
        //~ curr_ptr = (uint8*)ArchMemory::get3GBAdressOfPPN(page);
        //~ page_free = PAGE_SIZE;
      //~ }
      //~ if (still_to_read > 0)
      //~ {
        //~ *curr_ptr = file_image_[h->p_offset + read]; 
        //~ read++;
        //~ still_to_read--;
      //~ }
      //~ else
      //~ {
        //~ *curr_ptr = 0;
      //~ }
      //~ curr_ptr++;
      //~ still_to_write--;
      //~ ptr++;
      //~ page_free--;
    //~ }
  //~ }

  ArchThreads::createThreadInfosUserspaceThread(thread_->user_arch_thread_info_, hdr->e_entry, 2U*1024U*1024U*1024U-sizeof(pointer), thread_->getStackStartPointer());
  ArchThreads::setPageDirectory(thread_,page_dir_page_);

}

void Loader::loadOnePage(uint32 virtual_address)
{
  uint32 virtual_page = virtual_address / PAGE_SIZE;
  kprintfd("Loader::loadOnePage: going to load virtual page %d (virtual_address=%d)\n",virtual_page,virtual_address);
  

  //uint32 page_dir_page = ArchThreads::getPageDirectory(thread);

  ELF32_Ehdr *hdr = reinterpret_cast<ELF32_Ehdr *>(file_image_);
  
  kprintfd("Loader::loadOnePage: %c%c%c%c%c\n",file_image_[0],file_image_[1],file_image_[2],file_image_[3],file_image_[4]);
  kprintfd("Loader::loadOnePage: Sizeof %d %d %d %d\n",sizeof(uint64),sizeof(uint32),sizeof(uint16),sizeof(uint8));
  kprintfd("Loader::loadOnePage: Num ents: %d\n",hdr->e_phnum);
  kprintfd("Loader::loadOnePage: Entry: %x\n",hdr->e_entry);
  
  
  uint32 page = PageManager::instance()->getFreePhysicalPage();
  ArchMemory::mapPage(page_dir_page_, virtual_page, page, true);
  ArchCommon::bzero(ArchMemory::get3GBAdressOfPPN(page),PAGE_SIZE,false);
  
  pointer vaddr = virtual_page*PAGE_SIZE;

  bool wrote_someting=false;  
  
  uint32 i=0;
  uint32 read=0;
 
  for (i=0;i<hdr->e_phnum;++i)
  {
    ELF32_Phdr *h = (ELF32_Phdr *)((uint32)file_image_ + hdr->e_phoff + i* hdr->e_phentsize);
    kprintfd("Loader::loadOnePage: PHdr[%d].vaddr=%x .paddr=%x .type=%x .memsz=%x .filez=%x .poff=%x\r\n",i,h->p_vaddr,h->p_paddr,h->p_type,h->p_memsz,h->p_filesz,h->p_offset);
    
    //if (vaddr >= h->p_paddr && vaddr < h->p_paddr+h->p_memsz)
    if ((h->p_paddr >= vaddr && h->p_paddr < vaddr+PAGE_SIZE) || 
        (h->p_paddr+h->p_memsz >= vaddr && h->p_paddr+h->p_memsz < vaddr + PAGE_SIZE))
    {
      //now write from max(h->p_addr,vaddr) to min(h->p_addr+h->p_memsz,vaddr+PAGE_SIZE)
      kprintfd("Loader::loadOnePage: loading from PHdr[%d]\r\n",i);
      pointer write_start = vaddr;
      if (h->p_paddr > write_start)
        write_start=h->p_paddr;
      
      pointer write_stop = vaddr+PAGE_SIZE;
      if (h->p_paddr + h->p_memsz < write_stop)
        write_stop = h->p_paddr + h->p_memsz;
      
      uint8 *curr_ptr = reinterpret_cast<uint8*>(ArchMemory::get3GBAdressOfPPN(page) +  (write_start % PAGE_SIZE));
      
      if (h->p_paddr > vaddr)
        read=0;
      else
        read = vaddr - h->p_paddr;
   
      for (curr_ptr = reinterpret_cast<uint8*>(write_start); curr_ptr < reinterpret_cast<uint8*>(write_stop); ++curr_ptr)
      {
        *curr_ptr = file_image_[h->p_offset + read];
        ++read;
      }
      kprintfd("Loader::loadOnePage: wrote %d bytes\r\n",write_stop - write_start);
      wrote_someting=true;
    }
  }
  if (! wrote_someting)
  {
    //ERRRROOORRRR: we didn't load anything apparently, because no ELF section
    //corresponded with our vaddr    
    kpanict((uint8*) "Loader: loadOnePage(): we didn't load anything apparently\r\n");
    //kprintfd( "Loader: loadOnePage(): we didn't load anything apparently\n");
  }
}
