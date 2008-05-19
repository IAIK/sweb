
file(GLOB arch_xen_SOURCES ${SOURCE_WILDCARDS})

add_library(arch_xen ${arch_xen_SOURCES} ${ASM_OBJECTS})
set(ENV{LIBRARY_NAMES} "$ENV{LIBRARY_NAMES};arch_xen")
