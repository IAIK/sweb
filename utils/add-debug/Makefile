CXXFLAGS+=-g -O2 -fPIC
override CXXFLAGS+=-std=c++11 -Wall

CLEAN :=

all: libelf libdwarf add-dbg
# 	$(MAKE) -C elf
# 	$(MAKE) -C dwarf
# 	$(MAKE) add-dbg

libelf: 
	$(MAKE) -C elf
	
libdwarf: 
	$(MAKE) -C dwarf
	
CPPFLAGS+=-I./elf -I./dwarf
LDLIBS+=./elf/libelf++.a ./dwarf/libdwarf++.a

# Dependencies
CPPFLAGS+=-MD -MP -MF .$@.d
-include .*.d

add-dbg: add-dbg.o libdwarf libelf
	$(LINK.cc) add-dbg.o $(LOADLIBES) $(LDLIBS) -o $@
CLEAN += add-dbg add-dbg.o

clean:
	rm -f $(CLEAN) .*.d
