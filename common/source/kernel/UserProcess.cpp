#include "UserProcess.h"
#include "Console.h"
#include "File.h"
#include "Loader.h"
#include "PageManager.h"
#include "ProcessRegistry.h"
#include "VfsSyscall.h"
#include "kprintf.h"
#include "offsets.h"
#include "ArchMemory.h"
#include "ArchMulticore.h"
#include "ArchThreads.h"
#include "Scheduler.h"
#include "EASTL/string.h"

UserProcess::UserProcess(const eastl::string& executable_path,
                         FileSystemInfo* working_dir,
                         uint32 terminal_number) :
    Thread(working_dir, executable_path, Thread::USER_THREAD),
    fd_(VfsSyscall::open(executable_path.c_str(), O_RDONLY))
{
    debug(USERPROCESS, "Creating new user process %s\n", executable_path.c_str());
    // should also be called if you fork a process
    ProcessRegistry::instance()->processStart();

    if (fd_ >= 0)
        loader_ = new Loader(fd_);

    if (!loader_ || !loader_->loadExecutableAndInitProcess())
    {
        debug(USERPROCESS, "Error: loading %s failed!\n", executable_path.c_str());
        kill();
        return;
    }

    // Allocate a physical page for the stack and map it into the virtual address space
    size_t stack_ppn = PageManager::instance().allocPPN();
    size_t stack_vpn = (USER_BREAK / PAGE_SIZE) - 1;

    bool vpn_mapped = loader_->arch_memory_.mapPage(stack_vpn, stack_ppn, true);
    assert(vpn_mapped &&
           "Virtual page for stack was already mapped - this should never happen");

    debug(THREAD, "Mapped stack at virt [%zx, %zx) -> phys [%zx, %zx)\n",
          stack_vpn * PAGE_SIZE, (stack_vpn + 1) * PAGE_SIZE, stack_ppn * PAGE_SIZE,
          (stack_ppn + 1) * PAGE_SIZE);

    // Stack pointer is decremented as new items are pushed onto the stack -> start at
    // high end
    void* init_user_stackptr =
        (void*)((stack_vpn * PAGE_SIZE) + PAGE_SIZE - 2 * sizeof(pointer));

    user_registers_ = ArchThreads::createUserRegisters(
        loader_->getEntryFunction(), init_user_stackptr, getKernelStackStartPointer());

    // Ensure the thread is using the correct virtual memory address space
    ArchThreads::setAddressSpace(this, loader_->arch_memory_);

    debug(USERPROCESS, "ctor: Done loading %s\n", executable_path.c_str());

    if (main_console->getTerminal(terminal_number))
        setTerminal(main_console->getTerminal(terminal_number));

    // Run this thread in userspace when it is scheduled
    switch_to_userspace_ = 1;
}

UserProcess::~UserProcess()
{
    assert(Scheduler::instance()->isCurrentlyCleaningUp());
    delete loader_;
    loader_ = nullptr;

    if (fd_ > 0)
        VfsSyscall::close(fd_);

    delete working_dir_;
    working_dir_ = nullptr;

    ProcessRegistry::instance()->processExit();
}

void UserProcess::Run()
{
    debug(USERPROCESS, "Run: Fail-safe kernel panic - you probably have forgotten to set "
                       "switch_to_userspace_ = 1\n");
    assert(false && "UserProcess::Run called");
}
