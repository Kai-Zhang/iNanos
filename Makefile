CC      = gcc
LD      = ld
CFLAGS  = -m32 -static -ggdb -MD -Wall -Werror -I./include \
		 -fno-builtin -fno-stack-protector -fno-omit-frame-pointer
ASFLAGS = -ggdb -m32 -MD -I./include
LDFLAGS = -melf_i386
QEMU    = qemu-system-i386

CFILES  = $(shell find src/ -name "*.c")
SFILES  = $(shell find src/ -name "*.S")
OBJS    = $(CFILES:.c=.o) $(SFILES:.S=.o)

disk.img: kernel
	@cd boot; make
	cat boot/bootblock kernel > disk.img

kernel: $(OBJS)
	$(LD) $(LDFLAGS) -e os_init -Ttext 0xC0100000 -o kernel $(OBJS)

-include $(OBJS:.o=.d)

run:
	$(QEMU) -serial stdio disk.img

debug:
	$(QEMU) -serial stdio -s -S disk.img

clean:
	@cd boot; make clean
	rm -f kernel disk.img $(OBJS) $(OBJS:.o=.d)
