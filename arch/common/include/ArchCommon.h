#ifndef _ARCH_COMMON_H_
#define _ARCH_COMMON_H_

#include "types.h"
#include "paging-definitions.h"

class Console;

class ArchCommon
{
  public:
    static pointer getKernelEndAddress();
    static pointer getFreeKernelMemoryStart();
    static pointer getFreeKernelMemoryEnd();

    /**
     * @param is_paging_set_up inform function that paging is not yet up (=false) and
     * that it should behave accordingly
     */
    static uint32 haveVESAConsole(uint32 is_paging_set_up = 1);
    static uint32 getVESAConsoleHeight();
    static uint32 getVESAConsoleWidth();
    static uint32 getVESAConsoleBitsPerPixel();

    /**
     * @return a Pointer to the location of the VESA Memory Region
     */
    static pointer getVESAConsoleLFBPtr(uint32 is_paging_set_up = 1);

    /**
     * @return a Pointer to the location of the FrameBuffer
     */
    static pointer getFBPtr(uint32 is_paging_set_up = 1);

    /**
     * @return number of Useable Memory Regions
     */
    static uint32 getNumUseableMemoryRegions();

    /**
     * Reads the Grub MultiBoot-Info and
     * returns the start- and end-address of the Useable Memory Region x
     *
     * @param number of the region-information to parse
     * @param &start_address of Useable Memory Region
     * @param &end_address of Useable Memory Region
     * @param &type of Useable Memory Region
     * @return 1 if region >= number_of_regions, 0 otherwise
     */
    static uint32 getUsableMemoryRegion(size_t region, pointer &start_address, pointer &end_address, size_t &type);

    /**
     * @return uint32 returns the number of modules loaded by grub
     */
    static uint32 getNumModules(uint32 is_paging_set_up = 1);

    /**
     * Parses the Grub MultiBoot Info with regard to modules
     *
     * @param num the number of grub-loaded module which this is about
     * @return uint32 returns memory start address of module "num"
     */
    static uint32 getModuleStartAddress(uint32 num, uint32 is_paging_set_up = 1);

    /**
     * Parses the Grub MultiBoot Info with regard to modules
     *
     * @param num the number of grub-loaded module which this is about
     * @return uint32 returns memory end address of module "num"
     */
    static uint32 getModuleEndAddress(uint32 num, uint32 is_paging_set_up = 1);

    /**
     * Generates the according console depending on the architecture
     * @param count the number of consoles to create
     * @return a pointer to the Console object
     */
    static Console* createConsole(uint32 count);

    /**
     * Generates the Debug info depending on the architecture
     */
    static void initDebug();

    /**
     * let the CPU idle, f.e. with the halt statement
     */
    static void idle();

    /**
     * draw a heartbeat character
     */
    static void drawHeartBeat();
};

#endif
