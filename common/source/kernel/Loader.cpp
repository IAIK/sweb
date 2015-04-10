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

void Loader::loadPageFast(pointer virtual_address)
{
  MutexLock lock(program_binary_lock_);
  if(arch_memory_.checkAddressValid(virtual_address))
  {
    // the page has been mapped meanwhile
    return;
  }

  size_t ppn = PageManager::instance()->allocPPN();

  // TODO: Move the defines to a better address
  const pointer virt_page_start_addr = virtual_address & ~(PAGE_SIZE - 1);
  const pointer virt_page_end_addr = virt_page_start_addr + PAGE_SIZE;
  ustl::vector<Elf::Phdr>::iterator it;
  debug(THREAD, "Start %x, End %x, addr %x\n", virt_page_start_addr, virt_page_end_addr, virtual_address);

  bool page_in_binary = false;
  // load the parts from the binary

#define HERE() do{debug(BACKTRACE, "Here, line is %lu.\n", __LINE__);}while(0)
  // find the start point
  for(it = phdrs_.begin(); it != phdrs_.end(); it++)
  {

    debug(THREAD, "cleanAndSortHeaders: .vaddr=%x .paddr=%x .type=%x .flags=%x .memsz=%x .filez=%x .poff=%x, PAGE_START: %p, PAGE_END: %p, virt_addr: %p\r\n",
                 (*it).p_vaddr, (*it).p_paddr, (*it).p_type, (*it).p_flags, (*it).p_memsz, (*it).p_filesz, (*it).p_offset, virt_page_start_addr, virt_page_end_addr, virtual_address);

    // check if the section
    if(ADDRESS_BETWEEN((*it).p_vaddr, virt_page_start_addr, virt_page_end_addr) ||
       ADDRESS_BETWEEN((*it).p_vaddr + (*it).p_memsz, virt_page_start_addr , virt_page_end_addr) ||
       ADDRESS_BETWEEN((*it).p_vaddr + (*it).p_filesz, virt_page_start_addr , virt_page_end_addr) ||
       ADDRESS_BETWEEN(virt_page_start_addr, (*it).p_vaddr, (*it).p_vaddr + (*it).p_filesz) ||
       ADDRESS_BETWEEN(virt_page_start_addr, (*it).p_vaddr, (*it).p_vaddr + (*it).p_memsz))
    {
      page_in_binary = true;
      break;
    }
  }
  if(!page_in_binary)
  {
    HERE();
    PageManager::instance()->freePPN(ppn);
    program_binary_lock_.release();
    Syscall::exit(9999);
  }

  pointer bin_start_addr;
  pointer bin_end_addr;
  pointer virt_start_addr;
  pointer virt_end_addr;
  for(; it != phdrs_.end() && (*it).p_vaddr < virt_page_end_addr; it++)
  {
    HERE();
    if((*it).p_filesz && (*it).p_vaddr + (*it).p_filesz >= virt_page_start_addr)
    {
      virt_start_addr = ustl::max(virt_page_start_addr, (*it).p_vaddr);
      virt_end_addr = ustl::min(virt_page_end_addr, (*it).p_vaddr + (*it).p_filesz);
      bin_start_addr = (*it).p_offset + (virt_start_addr - (*it).p_vaddr);
      assert(!(virt_start_addr - virt_page_start_addr));
//      HERE();
//      // a file has to be load
//      // get the binary start address
//
//      if((*it).p_vaddr >= virt_page_start_addr)
//      {
//        HERE();
//        bin_start_addr = (*it).p_offset;
//        virt_start_addr = (*it).p_vaddr;
//      }
//      else
//      {
//        HERE();
//        const size_t diff = virt_page_start_addr - (*it).p_vaddr;
//        debug(BACKTRACE, "DIFF: %d\n", diff);
//        bin_start_addr = (*it).p_offset + diff;
//        virt_start_addr = virt_page_start_addr;
//      }
//
//      if(((*it).p_vaddr + (*it).p_filesz) < virt_page_end_addr)
//      {
//        HERE();
//        bin_end_addr = (*it).p_offset + (*it).p_filesz;
//        virt_size = (*it).p_filesz;
//      }
//      else
//      {
//        //((*it).p_vaddr + (*it).p_filesz) >= virt_page_end_addr
//        const size_t diff = ((*it).p_offset + (*it).p_filesz) - PAGE_SIZE;
//        debug(BACKTRACE, "DIFF2: %d\n", diff);
//        bin_end_addr = (*it).p_offset + (*it).p_filesz - diff;
//        virt_size = (*it).p_filesz - diff;
//      }
//      HERE();
      debug(THREAD, "Bin-start: %x, mem_start: %x, Bin-size: %x\n", bin_start_addr, (virt_start_addr - virt_page_start_addr), (virt_end_addr - virt_start_addr));
      readFromBinary((char *)ArchMemory::getIdentAddressOfPPN(ppn) + (virt_start_addr - virt_page_start_addr), bin_start_addr,  (virt_end_addr - virt_start_addr));
    }
  }
  arch_memory_.mapPage(virt_page_start_addr >> 12, ppn, true, PAGE_SIZE);
}

bool Loader::readFromBinary (char* buffer, l_off_t position, size_t count)
{
  debug(THREAD, "%x %x %x\n", buffer, position, count);
  VfsSyscall::lseek(fd_, position, SEEK_SET);
  return VfsSyscall::read(fd_, buffer, count) - (int32)count;
}

bool Loader::readHeaders()
{
  hdr_ = new Elf::Ehdr;

  VfsSyscall::lseek(fd_, 0, SEEK_SET);
  if(!hdr_ || VfsSyscall::read(fd_, reinterpret_cast<char*>(hdr_),
              sizeof(Elf::Ehdr)) != sizeof(Elf::Ehdr))
  {
    return false;
  }

  //checking elf-magic-numbers, format (32/64bit) and a few more things
  if (!Elf::headerCorrect(hdr_))
    return false;


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
  cleanAndSortHeaders();
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

  debug ( LOADER,"loadExecutableAndInitProcess: Entry: %x, num Sections %x\n",hdr_->e_entry, hdr_->e_phnum );
  if (LOADER & OUTPUT_ENABLED)
    Elf::printElfHeader ( *hdr_ );

  if (USERTRACE & OUTPUT_ENABLED)
    loadDebugInfoIfAvailable();

  return true;
}

struct PagePart
{
    size_t page_byte_;
    size_t vaddr_;
    size_t length_;
    PagePart(size_t page_byte, size_t vaddr, size_t length) : page_byte_(page_byte), vaddr_(vaddr), length_(length)
    {
    }
};

void Loader::loadPage(pointer virtual_address)
{
  loadPageFast(virtual_address); return;
  size_t virtual_page = virtual_address / PAGE_SIZE;

  MutexLock program_binary_lock(program_binary_lock_);
  //check if page has not been loaded meanwhile
  if (arch_memory_.checkAddressValid(virtual_address))
  {
    debug(LOADER, "loadPage: Page %d (virtual_address=%d) has already been mapped, probably by another thread between pagefault and reaching loader.\n", virtual_page, virtual_address);
    return;
  }

  debug(LOADER, "loadPage: going to load virtual page %d (virtual_address=%d) for %d:%s\n", virtual_page, virtual_address, currentThread->getTID(), currentThread->getName());

  ustl::vector<PagePart> byte_map;
  size_t min_byte_to_load = 0xFFFFFFFF;
  size_t max_byte_to_load = 0;
  size_t found = 0;
  if (virtual_address != 0)
  {
    for (size_t i = 0; i < PAGE_SIZE; ++i)
    {
      size_t load_byte_from_address = virtual_page * PAGE_SIZE + i;
      size_t k = 0;
      for (Elf::Phdr& h : phdrs_)
      {
        debug(LOADER, "loadPage: PHdr[%d].vaddr=%x .paddr=%x .type=%x .flags=%x .memsz=%x .filez=%x .poff=%x\r\n", k++, h.p_vaddr, h.p_paddr, h.p_type, h.p_flags, h.p_memsz, h.p_filesz, h.p_offset);

        if (ADDRESS_BETWEEN(load_byte_from_address, h.p_paddr, h.p_paddr + h.p_filesz))
        {
          size_t byte_to_load = h.p_offset + load_byte_from_address - h.p_paddr;
          size_t byte_count = (h.p_paddr + h.p_filesz) - load_byte_from_address;

          if (byte_count + i > PAGE_SIZE)
            byte_count = PAGE_SIZE - i;

          byte_map.push_back(PagePart(i, byte_to_load, byte_count));

          i += byte_count - 1;
          if(min_byte_to_load > byte_to_load) debug(THREAD, "Byte to load: %x, h.p_offset %x,  load_byte_from_address %x,  h.p_paddr %x\n", byte_to_load,  h.p_offset, load_byte_from_address, h.p_paddr);
          min_byte_to_load = Min(min_byte_to_load, byte_to_load);
          max_byte_to_load = Max(byte_to_load + byte_count, max_byte_to_load);

          ++found;
        }
        // bss is not in the file but in memory
        else if (ADDRESS_BETWEEN(load_byte_from_address, h.p_paddr, h.p_paddr + h.p_memsz))
        {
          // skip ahead to end of section
          i += (h.p_paddr + h.p_memsz) - load_byte_from_address - 1;
          debug ( LOADER,"In segment but not on file, this is .bss\n" );
          ++found;
        }
      }

      if (found > 1)
      {
        kprintfd("Loader::loadPage, byte (%x) in two different segments\n", load_byte_from_address);
      }
    }
  }

  if (!found)
  {
    kprintfd("Loader::loadPage: ERROR Request for Unknown Memory Location: v_adddr=%x, v_page=%d\n", virtual_address, virtual_page);
    program_binary_lock_.release();
    //free unmapped page
    Syscall::exit(9997);
  }

  size_t page = 0;
  //in this case all bytes are in bss-section, but not in file
  if (max_byte_to_load == 0 && min_byte_to_load == 0xffffffff)
  {
    debug(LOADER, "%x is in .bss\n", virtual_address);
    page = PageManager::instance()->allocPPN();
    memset((void*) ArchMemory::getIdentAddressOfPPN(page), 0, PAGE_SIZE);
    arch_memory_.mapPage(virtual_page, page, true);
    return;
  }

  //read once the bytes we need (and a few more, probably, depends on elf-format)
  size_t buffersize = max_byte_to_load - min_byte_to_load;
  uint8 buffer[PAGE_SIZE];
  assert(buffersize <= PAGE_SIZE && "this should never occur");

  debug(THREAD, "bin_start: %x, bin_size: %x\n", min_byte_to_load, max_byte_to_load - min_byte_to_load);
  VfsSyscall::lseek(fd_, min_byte_to_load, SEEK_SET);
  ssize_t bytes_read = VfsSyscall::read(fd_, (char*) buffer, max_byte_to_load - min_byte_to_load);

  if (bytes_read != static_cast<ssize_t>(max_byte_to_load - min_byte_to_load))
  {
    if (bytes_read == -1)
    {
      if (VfsSyscall::getFileDescriptor(fd_) == 0)
      {
        kprintfd("Loader::loadPage: ERROR cannot read from a closed file descriptor\n");
        assert(false);
      }
    }
    kprintfd("Loader::loadPage: ERROR part of executable not present in file: v_adddr=%x, v_page=%d\n", virtual_address, virtual_page);
    program_binary_lock_.release();
    Syscall::exit(9998);
  }
  page = PageManager::instance()->allocPPN();
  debug(PM, "got new page %x\n", page);
  memset((void*) ArchMemory::getIdentAddressOfPPN(page), 0, PAGE_SIZE);
  debug(PM, "bzero!\n");
  uint8* dest = reinterpret_cast<uint8*>(ArchMemory::getIdentAddressOfPPN(page));
  debug(PM, "copying %d elements\n", byte_map.size());
  size_t written = 0;
  for (PagePart& part : byte_map)
  {
    debug(PM, "copying from %x to %x ;   page byte: %d, length_: %d\n", buffer+part.vaddr_ - min_byte_to_load, dest + part.page_byte_, part.page_byte_, part.length_);

    assert(part.vaddr_ - min_byte_to_load + part.length_ <= buffersize);
    assert(part.page_byte_ + part.length_ <= PAGE_SIZE);
    memcpy(dest + part.page_byte_, buffer + part.vaddr_ - min_byte_to_load, part.length_);
    written += part.length_;
  }

  arch_memory_.mapPage(virtual_page, page, true);
  debug(PM, "loadPage: wrote a total of %d bytes\n", written);
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

void Loader::cleanAndSortHeaders()
{
  // sort the headers by the virtual start address
  // and throw away empty sections
  // TODO: To be implemented as lambda
  // now its just a simple sort
  {
    ustl::vector<Elf::Phdr>::iterator it1, it2;
    for(it1 = phdrs_.begin(); it1 != phdrs_.end() - 1; it1++)
    {
      for(it2 = it1 + 1; it2 != phdrs_.end(); it2++)
      {
        if((*it1).p_vaddr > (*it2).p_vaddr)
        {
          Elf::Phdr tmp = (*it1);
          (*it1) = (*it2);
          (*it2) = tmp;
        }
      }
    }

    // REMOVE EMPTY ELEMENTS
    // TODO: Rework
    for(it1 = phdrs_.begin(); it1 != phdrs_.end(); it1++)
    {
      debug(THREAD, "cleanAndSortHeaders: .vaddr=%x .paddr=%x .type=%x .flags=%x .memsz=%x .filez=%x .poff=%x\r\n",
                    (*it1).p_vaddr, (*it1).p_paddr, (*it1).p_type, (*it1).p_flags, (*it1).p_memsz, (*it1).p_filesz, (*it1).p_offset);
      // TODO: Define the types of phdr->p_type
      if(((*it1).p_memsz == 0 && (*it1).p_filesz == 0) || (*it1).p_type != 1)
      {
        it1 = phdrs_.erase(it1);
        it1--;
      }
      else
      {
        debug(THREAD, "cleanAndSortHeaders: .vaddr=%x .paddr=%x .type=%x .flags=%x .memsz=%x .filez=%x .poff=%x\r\n",
              (*it1).p_vaddr, (*it1).p_paddr, (*it1).p_type, (*it1).p_flags, (*it1).p_memsz, (*it1).p_filesz, (*it1).p_offset);
      }
    }
  }


}
