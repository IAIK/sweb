#pragma once

#include "Allocator.h"

namespace BootloaderModules
{
    void reserveModulePages(Allocator& allocator);
    void mapModules();
};
