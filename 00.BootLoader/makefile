all: BootLoader.bin

BootLoader.bin: BootLoader.asm
	nasm -o $@ $<

clean:
	rm -f BootLoader.bin