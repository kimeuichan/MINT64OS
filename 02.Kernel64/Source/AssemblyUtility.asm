[BITS 64]

SECTION .text

global kInPortByte, kOutPortByte

kInPortByte:
	push rdx

	mov rdx, rdi	; save param 1 (port number)
	mov rax, 0
	in al, dx		; read one byte from port address that is saved dx register

	pop rdx
	ret

kOutPortByte:
	push rdx
	push rax

	mov rdx, rdi	; save param 1 to rdx register
	mov rax, rsi	; save param 2 to rax register
	out dx, al		; write one byte to port address that is aved dx register

	pop rax
	pop rdx
	ret
