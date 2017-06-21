[BITS 64]

SECTION .text

extern Main

; Code Area
START:
	mov ax, 0x10		; store IA-32e mode kernel data segment descriptor to AX register
	mov dx, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	; create stack to 0x600000 ~ 0x6FFFFF
	mov ss, ax
	mov rsp, 0x6FFFF8
	mov rbp, 0x6FFFF8

	call Main

	jmp $
