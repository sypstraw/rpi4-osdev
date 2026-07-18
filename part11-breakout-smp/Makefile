CFILES = $(wildcard *.c lib/*.c)
OFILES = $(CFILES:.c=.o)
LLVMPATH = /opt/homebrew/opt/llvm/bin
LLDPATH = /opt/homebrew/opt/lld/bin
CLANGFLAGS = -Wall -O2 -ffreestanding -nostdinc -nostdlib -mcpu=cortex-a72+nosimd -mstrict-align

all: clean kernel8.img

boot/boot.o: boot/boot.S
	$(LLVMPATH)/clang --target=aarch64-elf $(CLANGFLAGS) -c $< -o $@

bin/BCM4345C0.o : bin/BCM4345C0.hcd
	$(LLVMPATH)/llvm-objcopy -I binary -O elf64-littleaarch64 -B aarch64 $< $@

bin/audio.o : bin/audio.bin
	$(LLVMPATH)/llvm-objcopy -I binary -O elf64-littleaarch64 -B aarch64 $< $@

%.o: %.c
	$(LLVMPATH)/clang --target=aarch64-elf $(CLANGFLAGS) -c $< -o $@

kernel8.img: boot/boot.o $(OFILES) bin/BCM4345C0.o bin/audio.o
	$(LLDPATH)/ld.lld -m aarch64elf -nostdlib boot/boot.o $(OFILES) bin/BCM4345C0.o bin/audio.o -T boot/link.ld -o kernel8.elf
	$(LLVMPATH)/llvm-objcopy -O binary kernel8.elf kernel8.img

clean:
	/bin/rm kernel8.elf *.o bin/*.o boot/*.o lib/*.o *.img > /dev/null 2> /dev/null || true
