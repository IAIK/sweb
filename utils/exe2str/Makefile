OBJS = $(patsubst %.c,%.o,$(wildcard *.c))

all: exe2str $(OBJS)
	./exe2str $(OBJS) > out.cstr

exe2str: exe2str.hs
	ghc -o exe2str exe2str.hs
	rm exe2str.o exe2str.hi

.PHONY: clean
clean:
	rm -f exe2str *.o *.hi out.cstr
