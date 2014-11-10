/**
 * @file Loader.cpp
 */

#include "Loader.h"
#include "console/kprintf.h"
#include "ArchThreads.h"
#include "mm/PageManager.h"
#include "ArchMemory.h"
#include "ArchCommon.h"
#include "ArchInterrupts.h"
#include "Syscall.h"
#include "fs/VfsSyscall.h"
#include <ustl/uvector.h>


Loader::Loader ( ssize_t fd, Thread *thread ) : fd_ ( fd ),
    thread_ ( thread ), hdr_(0), phdrs_(), load_lock_("Loader::load_lock_")
{
}

Loader::~Loader()
{
  delete hdr_;
}


void Loader::initUserspaceAddressSpace()
{
  size_t page_for_stack = PageManager::instance()->getFreePhysicalPage();

  arch_memory_.mapPage(1024*512-1, page_for_stack, 1); // (1024 * 512 - 1) * 4 KiB is exactly 2GiB - 4KiB
}

bool Loader::readHeaders()
{
  //the ehdr and the phdrs are saved as members, since they
  //are often needed

  hdr_ = new Elf::Ehdr;

  VfsSyscall::instance()->lseek(currentThread->getWorkingDirInfo(), fd_, 0, SEEK_SET);
  if(!hdr_ || VfsSyscall::instance()->read(currentThread->getWorkingDirInfo(), fd_, reinterpret_cast<char*>(hdr_),
              sizeof(Elf::Ehdr)) != sizeof(Elf::Ehdr))
  {
    return false;
  }

  if (!Elf::headerCorrect(hdr_))
  //checking elf-magic-numbers, format (32/64bit) and a few more things
  {
    return false;
  }

  if(sizeof(Elf::Phdr) != hdr_->e_phentsize)
  {
    return false;
  }

  phdrs_.resize(hdr_->e_phnum, true);

  VfsSyscall::instance()->lseek(currentThread->getWorkingDirInfo(), fd_, hdr_->e_phoff, SEEK_SET);

  if(VfsSyscall::instance()->read(currentThread->getWorkingDirInfo(), fd_, reinterpret_cast<char*>(&phdrs_[0]), hdr_->e_phnum*sizeof(Elf::Phdr))
      != static_cast<ssize_t>(sizeof(Elf::Phdr)*hdr_->e_phnum))
  {
    return false;
  }

  return true;
}

bool Loader::loadExecutableAndInitProcess()
{
  debug ( LOADER,"Loader::loadExecutableAndInitProcess: going to load an executable\n" );

  initUserspaceAddressSpace();

  if(!readHeaders())
    return false;

  debug ( LOADER,"loadExecutableAndInitProcess: Entry: %x, num Sections %x\n",hdr_->e_entry, hdr_->e_phnum );
  if ( isDebugEnabled ( LOADER ) )
    Elf::printElfHeader ( *hdr_ );

  ArchThreads::createThreadInfosUserspaceThread (
        thread_->user_arch_thread_info_,
        hdr_->e_entry,
        2U*1024U*1024U*1024U - sizeof ( pointer ),
        thread_->getStackStartPointer()
  );

  ArchThreads::setAddressSpace(thread_, arch_memory_);

  return true;
}

struct PagePart
{
  size_t page_byte;
  size_t vaddr;
  size_t length;
};

#define ADDRESS_BETWEEN(Value, LowerBound, UpperBound) \
  ((((size_t)Value) >= ((size_t)LowerBound)) && (((size_t)Value) < ((size_t)UpperBound)))

void Loader::loadOnePageSafeButSlow ( pointer virtual_address )
{
  size_t virtual_page = virtual_address / PAGE_SIZE;

  MutexLock loadlock(load_lock_);
  //check if page has not been loaded meanwhile
  if(arch_memory_.checkAddressValid(virtual_address))
  {
    debug ( LOADER,"loadOnePageSafeButSlow: Page %d (virtual_address=%d) has already been mapped, probably by another thread between pagefault and reaching loader.\n",virtual_page,virtual_address );
    return;
  }

  debug ( LOADER,"loadOnePageSafeButSlow: going to load virtual page %d (virtual_address=%d) for %d:%s\n",virtual_page,virtual_address,currentThread->getPID(),currentThread->getName() );

  debug ( LOADER,"loadOnePage: Num ents: %d\n",hdr_->e_phnum );
  debug ( LOADER,"loadOnePage: Entry: %x\n",hdr_->e_entry );

  size_t page = 0;
  size_t i=0;
  size_t k=0;
  size_t written=0;

  ustl::vector<PagePart> byte_map;
  PagePart part;
  size_t min_value = 0xFFFFFFFF;
  size_t max_value = 0;
  if (virtual_address != 0)
  {
    for ( i=0; i < PAGE_SIZE; ++i )
    {
      size_t load_byte_from_address = virtual_page*PAGE_SIZE + i;
      size_t found = 0;
      for ( k=0;k < hdr_->e_phnum; ++k )
      {
        Elf::Phdr& h = phdrs_[k];

        debug ( LOADER,"loadOnePage: PHdr[%d].vaddr=%x .paddr=%x .type=%x .memsz=%x .filez=%x .poff=%x\r\n",k,h.p_vaddr,h.p_paddr,h.p_type,h.p_memsz,h.p_filesz,h.p_offset );

        if (ADDRESS_BETWEEN(load_byte_from_address, h.p_paddr, h.p_paddr + h.p_filesz))
        {
          size_t byte_to_load = h.p_offset + load_byte_from_address - h.p_paddr;

          if(byte_to_load < min_value)
            min_value = byte_to_load;

          size_t load_count = (h.p_paddr + h.p_filesz) - load_byte_from_address;
          if (load_count + i > PAGE_SIZE)
              load_count = PAGE_SIZE - i;
          part.page_byte = i;
          part.vaddr = byte_to_load;
          part.length = load_count;
          i += load_count - 1;

          if(byte_to_load + load_count > max_value)
            max_value = byte_to_load + load_count;

          //its VERY MUCH more efficient to only search the bytes, save them in the array byte_map,
          //and read ONCE from the executable; its very expensive to read every byte single from harddisk
          //(we have to read a full zone) -> for this we also need the max- and min-byte from file we have to load

          byte_map.push_back(part);

          ++written;
          ++found;
        }
        // bss is not in the file but in memory
        else if (ADDRESS_BETWEEN(load_byte_from_address, h.p_paddr, h.p_paddr + h.p_memsz))
        {
          // skip ahead to end of section
          i += (h.p_paddr + h.p_memsz) - load_byte_from_address - 1;
          debug ( LOADER,"In segment but not on file, this is .bss\n" );
          ++found;
          ++written;
        }
      }

      if ( !found )
      {
        debug ( LOADER,"Byte not found, byte virtual address is %x\n",load_byte_from_address );
        // this is expected behaviour. Our Loader tries to find every bytes on a needed page in the ELF Header.
        // Of course, often only parts of a page are listed in the ELF Hedaer
      }
      else if ( found >1 )
      {
        kprintfd ( "Loader::loadOnePageSafeButSlow:EEEEEEEEEEEERRRRRRRROR, found the byte (%x) in two different segments\n", load_byte_from_address );
      }
    }
  }

  if ( !written )
  {
    kprintfd ( "Loader::loadOnePageSafeButSlow: ERROR Request for Unknown Memory Location: v_adddr=%x, v_page=%d\n",virtual_address,virtual_page);
    load_lock_.release();
    //free unmapped page
    Syscall::exit ( 9997 );
  }

  //in this case all bytes are in bss-section, but not in file
  if(max_value == 0 && min_value == 0xffffffff)
  {
    debug(LOADER, "%x is in .bss\n", virtual_address);
    page = PageManager::instance()->getFreePhysicalPage();
    ArchCommon::bzero ( ArchMemory::getIdentAddressOfPPN ( page ),PAGE_SIZE,false );
    arch_memory_.mapPage(virtual_page, page, true);
    return;
  }

  //read once the bytes we need (and a few more, probably, depends on elf-format)
  size_t buffersize = max_value - min_value;
  uint8* buffer = 0;
  uint8 page_buffer[PAGE_SIZE];
  assert(buffersize <= PAGE_SIZE && "how could the other case occur and how would we handle it?");
  if (buffersize <= PAGE_SIZE)
  {
    buffer = page_buffer;
  }
  else
  {
    buffer = new uint8[buffersize];
  }
  debug(PM, "buffer is %d bytes long\n", buffersize);

  if(!buffer)
  {
    kprintfd ( "Loader::loadOnePageSafeButSlow: ERROR not enough heap memory\n");
    load_lock_.release();
    //free unmapped page
    Syscall::exit ( 9996 );
  }


  VfsSyscall::instance()->lseek(currentThread->getWorkingDirInfo(), fd_, min_value, SEEK_SET);
  ssize_t bytes_read = VfsSyscall::instance()->read(currentThread->getWorkingDirInfo(), fd_, (char*)buffer, max_value - min_value);

  if(bytes_read != static_cast<ssize_t>(max_value - min_value))
  {
    if (bytes_read == -1)
    {
      if (FileDescriptor::getFileDescriptor(fd_) == NULL)
      {
        kprintfd("Loader::loadOnePageSafeButSlow: ERROR cannot read from a closed file descriptor\n");
        assert(false);
      }
    }
    kprintfd ( "Loader::loadOnePageSafeButSlow: ERROR part of executable not present in file: v_adddr=%x, v_page=%d\n", virtual_address, virtual_page);
    //free buffer
    if (buffersize > PAGE_SIZE)
      delete[] buffer;
    load_lock_.release();
    Syscall::exit ( 9998 );
   }
  page = PageManager::instance()->getFreePhysicalPage();
  debug(PM, "got new page %x\n", page);
  ArchCommon::bzero ( ArchMemory::getIdentAddressOfPPN ( page ),PAGE_SIZE,false );
  debug(PM, "bzero!\n");
  uint8* dest = reinterpret_cast<uint8*> (ArchMemory::getIdentAddressOfPPN ( page ));
  debug(PM, "copying %d elements\n", byte_map.size());
  written = 0;
  for(i=0; i < byte_map.size(); i++)
  {
    part = byte_map[i];

    debug(PM, "copying %dth element from %x to %x ;   page byte: %d, length: %d\n", i, buffer+part.vaddr - min_value, dest + part.page_byte, part.page_byte, part.length);

    assert(part.vaddr - min_value + part.length <= buffersize);
    assert(part.page_byte + part.length <= PAGE_SIZE);
    memcpy(dest + part.page_byte, buffer + part.vaddr - min_value, part.length);
    written += part.length;
  }

  if (buffersize > PAGE_SIZE)
    delete[] buffer;

  arch_memory_.mapPage(virtual_page, page, true);
  debug ( PM,"loadOnePageSafeButSlow: wrote a total of %d bytes\n",written );

}
