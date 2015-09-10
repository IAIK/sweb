#include "Loader.h"
#include "kprintf.h"
#include "ArchThreads.h"
#include "PageManager.h"
#include "ArchMemory.h"
#include "kstring.h"
#include "ArchInterrupts.h"
#include "Syscall.h"
#include "VfsSyscall.h"
#include <uvector.h>
#include "backtrace.h"
#include "Stabs2DebugInfo.h"
#include <umemory.h>
#include "File.h"
#include "FileDescriptor.h"

Loader::Loader(ssize_t fd) : fd_(fd), hdr_(0), phdrs_(), program_binary_lock_("Loader::load_lock_"), userspace_debug_info_(0)
{
}

Loader::~Loader()
{
  delete userspace_debug_info_;
  delete hdr_;
}

void Loader::loadPage(pointer virtual_address)
{
  MutexLock lock(program_binary_lock_);
  debug(LOADER, "Loader:loadPage: Request to load the page for address %p.\n", (void*)virtual_address);
  if(arch_memory_.checkAddressValid(virtual_address))
  {
    debug(LOADER, "Loader::loadPage: The page has been mapped by someone else.\n");
    program_binary_lock_.release();
    return;
  }
  const pointer virt_page_start = virtual_address & ~(PAGE_SIZE - 1);
  const pointer virt_page_end = virt_page_start + PAGE_SIZE;
  ustl::list<Elf::Phdr>::iterator it;
  // Find the first section which intersects with this page.
  it = phdrs_.find_if([&](const Elf::Phdr & el) -> bool
       {
         return (el.p_paddr < virt_page_end) && ((el.p_vaddr + ustl::max(el.p_memsz, el.p_filesz)) > virt_page_start);
       });

  if(it == phdrs_.end())
  {
    debug(LOADER, "Loader::loadPage: ERROR! No section refers to the given address.\n");
    program_binary_lock_.release();
    Syscall::exit(666);
  }
  // get a new page for the mapping
  size_t ppn = PageManager::instance()->allocPPN();
  // Iterate through all sections and load them into the page.
  // The rest of the page is automatically zeroed when the page is allocated.
  // The sections are sorted, so there is no need to check the sections out of range.
  for(; it != phdrs_.end() && (*it).p_vaddr < virt_page_end; it++)
  {
    if((*it).p_filesz && (*it).p_vaddr + (*it).p_filesz >= virt_page_start)
    {
      const pointer virt_start_addr = ustl::max(virt_page_start, (*it).p_vaddr);
      const size_t  bin_start_addr = (*it).p_offset + (virt_start_addr - (*it).p_vaddr);
      const size_t  bytes_to_load = ustl::min(virt_page_end, (*it).p_vaddr + (*it).p_filesz) - virt_start_addr;
      const size_t  virt_offs = virt_start_addr - virt_page_start;
      //debug(LOADER, "Loader::loadPage: Loading %d bytes from binary address %p to virtual address %p\n",
      //      bytes_to_load, bin_start_addr, virt_start_addr);
      readFromBinary((char *)ArchMemory::getIdentAddressOfPPN(ppn) + virt_offs, bin_start_addr,  bytes_to_load);
    }
  }
  arch_memory_.mapPage(virt_page_start / PAGE_SIZE, ppn, true, PAGE_SIZE);
  debug(LOADER, "Loader:loadPage: Load request for address %p has been successfully finished.\n", (void*)virtual_address);
}
bool Loader::readFromBinary (char* buffer, l_off_t position, size_t count)
{
  VfsSyscall::lseek(fd_, position, SEEK_SET);
  return VfsSyscall::read(fd_, buffer, count) - (int32)count;
}

bool Loader::readHeaders()
{
  hdr_ = new Elf::Ehdr;

  if(readFromBinary((char*)hdr_, 0, sizeof(Elf::Ehdr)))
  {
    debug(LOADER, "Loader::readHeaders: ERROR! The headers could not be load.\n");
    return false;
  }

  //checking elf-magic-numbers, format (32/64bit) and a few more things
  if (!Elf::headerCorrect(hdr_))
  {
    debug(LOADER, "Loader::readHeaders: ERROR! The headers are invalid.\n");
    return false;
  }

  if(sizeof(Elf::Phdr) != hdr_->e_phentsize)
  {
    debug(LOADER, "Expected program header size does not match advertised program header size\n");
    return false;
  }
  phdrs_.resize(hdr_->e_phnum, true);
  if(readFromBinary(reinterpret_cast<char*>(&phdrs_[0]), hdr_->e_phoff, hdr_->e_phnum*sizeof(Elf::Phdr)))
  {
    return false;
  }
  if(!cleanAndSortHeaders())
  {
    debug(LOADER, "Loader::readHeaders: ERROR! There are no valid sections in the elf file.\n");
    return false;
  }
  return true;
}

void* Loader::getEntryFunction() const
{
  return (void*)hdr_->e_entry;
}

bool Loader::loadExecutableAndInitProcess()
{
  debug ( LOADER,"Loader::loadExecutableAndInitProcess: going to load an executable\n" );

  if(!readHeaders())
    return false;

  debug(LOADER, "loadExecutableAndInitProcess: Entry: %zx, num Sections %x\n", hdr_->e_entry, hdr_->e_phnum);
  if (LOADER & OUTPUT_ENABLED)
    Elf::printElfHeader ( *hdr_ );

  if (USERTRACE & OUTPUT_ENABLED)
    loadDebugInfoIfAvailable();

  return true;
}

bool Loader::loadDebugInfoIfAvailable()
{
  assert(!userspace_debug_info_ && "You may not load User Debug Info twice!");

  debug(USERTRACE, "loadDebugInfoIfAvailable start\n");
  if (sizeof(Elf::Shdr) != hdr_->e_shentsize)
  {
    debug(USERTRACE, "Expected section header size does not match advertised section header size\n");
    return false;
  }

  ustl::vector<Elf::Shdr> section_headers;
  section_headers.resize(hdr_->e_shnum, true);
  if (readFromBinary(reinterpret_cast<char*>(&section_headers[0]), hdr_->e_shoff, hdr_->e_shnum*sizeof(Elf::Shdr)))
  {
    debug(USERTRACE, "Failed to load section headers!\n");
    return false;
  }

  // now that we have loaded the section headers, we want to find and load the section that contains
  // the section names
  // in the simple case this section name section is only 0xFF00 bytes long, in that case
  // loading is simple. we only support this case for now

  size_t section_name_section = hdr_->e_shstrndx;
  size_t section_name_size = section_headers[section_name_section].sh_size;
  ustl::vector<char> section_names(section_name_size);

  if (readFromBinary(&section_names[0], section_headers[section_name_section].sh_offset, section_name_size ))
  {
    debug(USERTRACE, "Failed to load section name section\n");
    return false;
  }

  // now that we have names we read through all the sections
  // and load the two we're interested in

  char *stab_data=0;
  char *stabstr_data=0;
  size_t stab_data_size=0;

  for (Elf::Shdr const &section: section_headers)
  {
    if (section.sh_name)
    {
      if (!strcmp(&section_names[section.sh_name], ".stab"))
      {
        debug(USERTRACE, "Found stab section, index is %d\n", section.sh_name);
        if (stab_data)
        {
          debug(USERTRACE, "Already loaded the stab section?, skipping\n");
        }
        else
        {
          size_t size = section.sh_size;
          stab_data = new char[size];
          stab_data_size = size;
          if (readFromBinary(stab_data, section.sh_offset, size))
          {
            debug(USERTRACE, "Failed to load stab section!\n");
            delete[] stab_data;
            stab_data=0;
          }
        }
      }
      if (!strcmp(&section_names[section.sh_name], ".stabstr"))
      {
        debug(USERTRACE, "Found stabstr section, index is %d\n", section.sh_name);
        if (stabstr_data)
        {
          debug(USERTRACE, "Already loaded the stabstr section?, skipping\n");
        }
        else
        {
          size_t size = section.sh_size;
          stabstr_data = new char[size];
          if (readFromBinary(stabstr_data, section.sh_offset, size))
          {
            debug(USERTRACE, "Failed to load stabstr section!\n");
            delete[] stabstr_data;
            stabstr_data=0;
          }
        }
      }
    }
  }

  if (!stab_data || !stabstr_data)
  {
    delete[] stab_data;
    delete[] stabstr_data;
    debug(USERTRACE, "Failed to load necessary debug data!\n");
    return false;
  }

  userspace_debug_info_ = new Stabs2DebugInfo(stab_data, stab_data + stab_data_size, stabstr_data);

  return true;
}

Stabs2DebugInfo const *Loader::getDebugInfos()const
{
  return userspace_debug_info_;
}

bool Loader::cleanAndSortHeaders()
{
  // sort the headers by the virtual start address
  // and throw away empty sections
  // as well as all sections which shall not be load
  phdrs_.sort([](const Elf::Phdr & elem1,const Elf::Phdr & elem2) -> bool
    {
      return elem1.p_vaddr < elem2.p_vaddr;
    });

  phdrs_.remove_if([](const Elf::Phdr & elem) -> bool
    {
      return !((elem.p_memsz == 0 && elem.p_filesz == 0) || elem.p_type != 1);
    });
  return phdrs_.size();

}
