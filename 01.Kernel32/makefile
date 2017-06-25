##################################################
# 빌드 환경 및 규칙 설정
##################################################
NASM=nasm
NASM32=nasm -f elf32
GCC32=x86_64-pc-linux-gcc -m32 -ffreestanding
LD32=x86_64-pc-linux-ld -melf_i386 -T ../elf_i386.x -nostdlib -e Main -Ttext 0x10200
OBJCOPY32=x86_64-pc-linux-objcopy -j .text -j .data -j .rodata -j .bss -S -O binary

SRC_DIR=Source
OBJ_DIR=Temp

##################################################
# 빌드 항목 및 빌드 방법 설정
##################################################
all: prepare Kernel32.bin

prepare:
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/EntryPoint.bin: $(SRC_DIR)/EntryPoint.s
	$(NASM) -o $@ $<

dep:
	make -C $(OBJ_DIR) -f ../Makefile InternalDependency

ExecuteInternalBuild: dep
	make -C $(OBJ_DIR) -f ../Makefile Kernel32.elf
	
$(OBJ_DIR)/Kernel32.elf.bin: ExecuteInternalBuild
	$(OBJCOPY32) $(OBJ_DIR)/Kernel32.elf $@

Kernel32.bin: $(OBJ_DIR)/EntryPoint.bin $(OBJ_DIR)/Kernel32.elf.bin
	cat $^ > $@

clean:
	rm -f *.bin
	rm -f $(OBJ_DIR)/*.*
	
##################################################
# Make에 의해 다시 호출되는 부분, Temp 디렉토리를 기준으로 수행됨.
##################################################
C_EP_OBJ=Main.o
C_SRCS=$(wildcard ../$(SRC_DIR)/*.c)
ASM_SRCS=$(wildcard ../$(SRC_DIR)/*.asm)
C_OBJS=$(subst $(C_EP_OBJ), , $(notdir $(patsubst %.c, %.o, $(C_SRCS))))
ASM_OBJS=$(notdir $(patsubst %.asm, %.o, $(ASM_SRCS)))

%.o: ../$(SRC_DIR)/%.c
	$(GCC32) -c $<

%.o: ../$(SRC_DIR)/%.asm
	$(NASM32) -o $@ $<

InternalDependency:
	$(GCC32) -MM $(C_SRCS) > Dependency.dep
	
Kernel32.elf: $(C_EP_OBJ) $(C_OBJS) $(ASM_OBJS)
	$(LD32) -o $@ $^
 
ifeq (Dependency.dep, $(wildcard Dependency.dep))
include Dependency.dep
endif