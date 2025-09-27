CFLAGS=-Ofast -march=i386 -mtune=i586 -Wall #-fno-loop-interchange
PREFIX=i586-pc-msdosdjgpp-
CC=$(PREFIX)gcc

#BUILD_DATE=$(shell date -u +'%y%W')
#CFLAGS+=-DBUILD_DATE=\"$(BUILD_DATE)\"
#CFLAGS+=-DGIT_HASH=\"$(shell git describe --always --dirty)\"

src=$(wildcard *.c)
obj=$(src:.c=.o)

TARGETS=bench.exe slack.exe demo.exe

.PHONY: all clean
all: bench.exe slack.exe demo.exe
clean:
	rm -f $(TARGETS) $(obj)

bench.exe: bench.c vga_dj.h
	$(CC) $(CFLAGS) $(LDFLAGS) -lemu $< -o $@
	$(CC) $(CFLAGS) -S -fverbose-asm -masm=intel $<
	@du -b $@
	@upx --best $@

slack.exe: slack.c vga_dj.h
	$(CC) -g -save-temps=obj $(CFLAGS) $(LDFLAGS) $< -o $@
	$(PREFIX)objdump -drwC -Mintel $(<:.c=.o) >$(<:.c=.s)
	@du -b $@
	@upx --best $@

demo.exe: demo.c vga_dj.h fixedpoint.h timer.h segiconcolor.h
	$(CC) -save-temps=obj $(CFLAGS) $(LDFLAGS) $< -o $@
	#$(PREFIX)objdump -drwC -Mintel $(<:.c=.o) >$(<:.c=.s)
	@du -b $@
	@upx --best $@

install: $(TARGETS)
	cp -p $^ ~/Emulation/DOS/C/code/
