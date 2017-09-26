[ORG 0x00]
[BITS 16]

SECTION .text

START:
	mov ax, 0x1000 ; º¸È£¸ðµå Ä¿³ÎÀÇ ¿£Æ®¸® Æ÷ÀÎÆ® ¸Þ¸ð¸® ¾îµå·¹½º(0x10000)
	mov ds, ax
	mov es, ax

	; Application Processor 이면 아래의 과정을 모두 뛰어넘어서 보호 모드 커널로 이동
	mov ax, 0x0000 			; application processor flag 확인하려고
	mov es, ax				; es 세그먼트 레지스터의 시작 어드레스를 0으로 설정

	cmp byte [es:0x7c09], 0x00 	; 플래그가 0이면 AP 이므로
	je .APPLICATIONPROCESSORSTARTPOINT ; AP용 코드로 이동

	; BootStrap Proccessor 만 실행되는 부분

	; A20 °ÔÀÌÆ® È°¼ºÈ­
	mov ax, 0x2401        ; ±â´É ¹øÈ£(0x2401:A20 °ÔÀÌÆ® È°¼ºÈ­)
	int 0x15              ; ÀÎÅÍ·´Æ® º¤ÅÍ Å×ÀÌºí ÀÎµ¦½º(0x15:BIOS Service->System Service)
	jc .A20_GATE_ERROR    ; ¿¹¿Ü ¹ß»ý½Ã Ã³¸®
	jmp .A20_GATE_SUCCESS ; ¼º°ø½Ã Ã³¸®

.A20_GATE_ERROR:
	in al, 0x92  ; ½Ã½ºÅÛ ÄÁÆ®·Ñ Æ÷Æ®(0x92)¿¡¼­ 1byteÀ» ÀÐ¾î AL¿¡ ÀúÀå
	or al, 0x02  ; A20 °ÔÀÌÆ® ºñÆ®(ºñÆ® 1)À» 1·Î ¼³Á¤
	and al, 0xFE ; ½Ã½ºÅÛ ¸®¼Â ºñÆ®(ºñÆ® 0)À» 0À¸·Î ¼³Á¤
	out 0x92, al ; ½Ã½ºÅÛ ÄÁÆ®·Ñ Æ÷Æ®(0x92)¿¡ º¯°æµÈ °ªÀ» ÀúÀå

.A20_GATE_SUCCESS:
.APPLICATIONPROCESSORSTARTPOINT:
	; º¸È£¸ðµå ÀüÈ¯
	cli                 ; ÀÎÅÍ·´Æ®°¡ ¹ß»ýÇÏÁö ¸øÇÏµµ·Ï ¼³Á¤
	lgdt [GDTR]         ; GDTR ÀÚ·á±¸Á¶¸¦ ÇÁ·Î¼¼¼­¿¡ ¼³Á¤ÇÏ¿© GDT Å×ÀÌºí ·Îµå
	mov eax, 0x4000003B ; CR0 ÄÁµå·Ñ ·¹Áö½ºÅÍ(PG=0, CD=1, NW=0, AM=0, WP=0, NE=1, ET=1, TS=1, EM=0, MP=1, PE=1)
	mov cr0, eax        ; ¼³Á¤°ªÀ» CR0¿¡ ÀúÀå
	jmp dword 0x18:(PROTECT_MODE - $$ + 0x10000) ; CS ¼¼±×¸ÕÆ® ¼¿·ºÅÍ¿¡ º¸È£¸ðµå¿ë ÄÚµå ¼¼±×¸ÕÆ® µð½ºÆ®¸³ÅÍ¸¦ ¼³Á¤ÇÏ°í, PROTECT_MODEÀÇ ¸Þ¸ð¸® ¾îµå·¹½º·Î ÀÌµ¿

[BITS 32]
PROTECT_MODE:
	mov ax, 0x20
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	; Application Processor 모두 과정을 뛰어넘어 바로 c커널 엔트리 포인트로 이동
	cmp byte[0x7c09], 0x00
	je .APPLICATIONPROCESSORSTARTPOINT

	mov esp, 0xFFFE
	mov ebp, 0xFFFE

	push (SWITCH_SUCCESS_MESSAGE - $$ + 0x10000)
	push 2
	push 0
	call PRINT_MESSAGE
	add esp, 12

.APPLICATIONPROCESSORSTARTPOINT:
	jmp dword 0x18:0x10200 ; CS ¼¼±×¸ÕÆ® ¼¿·ºÅÍ¿¡ º¸È£¸ðµå¿ë ÄÚµå ¼¼±×¸ÕÆ® µð½ºÆ®¸³ÅÍ¸¦ ¼³Á¤ÇÏ°í, C¾ð¾î ¿£Æ®¸® Æ÷ÀÎÆ® ÇÔ¼öÀÇ ¸Þ¸ð¸® ¾îµå·¹½º(0x10200)·Î ÀÌµ¿

PRINT_MESSAGE:
	push ebp
	mov ebp, esp
	push esi
	push edi
	push eax
	push ecx
	push edx

	mov eax, dword [ebp + 12]
	mov esi, 160
	mul esi
	mov edi, eax

	mov eax, dword [ebp + 8]
	mov esi, 2
	mul esi
	add edi, eax

	mov esi, dword [ebp + 16]

.MESSAGE_LOOP:
	mov cl, byte [esi]
	cmp cl, 0
	je .MESSAGE_END
	mov byte [edi + 0xB8000], cl
	mov byte [edi + 0xB8000+1], 0x07
	add esi, 1
	add edi, 2
	jmp .MESSAGE_LOOP

.MESSAGE_END:
	pop edx
	pop ecx
	pop eax
	pop edi
	pop esi
	pop ebp
	ret

align 8, db 0 ; ¾Æ·¡ÀÇ µ¥ÀÌÅÍ¸¦ 8byte Å©±â·Î Á¤·Ä

dw 0x0000 ; GDTR¸¦ 8byte Å©±â·Î Á¤·ÄÇÏ±â À§ÇÏ¿© Ãß°¡

GDTR:
	dw GDT_END - GDT - 1    ; GDT Size(2byte)
	dd (GDT - $$ + 0x10000) ; GDT BaseAddress(4byte)

GDT:
	NULL_DESCRIPTOR: ; ³Î ¼¼±×¸ÕÆ® µð½ºÅ©¸³ÅÍ(8byte)
		dw 0x0000
		dw 0x0000
		db 0x00
		db 0x00
		db 0x00
		db 0x00

	IA32E_CODE_DESCRIPTOR: ; IA-32e ¸ðµå¿ë ÄÚµå ¼¼±×¸ÕÆ® µð½ºÅ©¸³ÅÍ(8byte)
		dw 0xFFFF    ; Limit=0xFFFF
		dw 0x0000    ; BaseAddress=0x0000
		db 0x00      ; BaseAddress=0x00
		db 0x9A      ; P=1, DPL=00, S=1, Type=0xA:CodeSegment(½ÇÇà/ÀÐ±â)
		db 0xAF      ; G=1, D/B=0, L=1, AVL=0, Limit=0xF
		db 0x00      ; BaseAddress=0x00

	IA32E_DATA_DESCRIPTOR: ; IA-32e ¸ðµå¿ë  µ¥ÀÌÅÍ ¼¼±×¸ÕÆ® µð½ºÅ©¸³ÅÍ(8byte)
		dw 0xFFFF    ; Limit=0xFFFF
		dw 0x0000    ; BaseAddress=0x0000
		db 0x00      ; BaseAddress=0x00
		db 0x92      ; P=1, DPL=00, S=1, Type=0x2:DataSegment(ÀÐ±â/¾²±â)
		db 0xAF      ; G=1, D/B=0, L=1, AVL=0, Limit=0xF
		db 0x00      ; BaseAddress=0x00

	PROTECT_CODE_DESCRIPTOR: ; º¸È£ ¸ðµå¿ë ÄÚµå ¼¼±×¸ÕÆ® µð½ºÅ©¸³ÅÍ(8byte)
		dw 0xFFFF    ; Limit=0xFFFF
		dw 0x0000    ; BaseAddress=0x0000
		db 0x00      ; BaseAddress=0x00
		db 0x9A      ; P=1, DPL=00, S=1, Type=0xA:CodeSegment(½ÇÇà/ÀÐ±â)
		db 0xCF      ; G=1, D/B=1, L=0, AVL=0, Limit=0xF
		db 0x00      ; BaseAddress=0x00

	PROTECT_DATA_DESCRIPTOR: ; º¸È£ ¸ðµå¿ë µ¥ÀÌÅÍ ¼¼±×¸ÕÆ® µð½ºÅ©¸³ÅÍ(8byte)
		dw 0xFFFF    ; Limit=0xFFFF
		dw 0x0000    ; BaseAddress=0x0000
		db 0x00      ; BaseAddress=0x00
		db 0x92      ; P=1, DPL=00, S=1, Type=0x2:DataSegment(ÀÐ±â/¾²±â)
		db 0xCF      ; G=1, D/B=1, L=0, AVL=0, Limit=0xF
		db 0x00      ; BaseAddress=0x00

GDT_END:

SWITCH_SUCCESS_MESSAGE: db 'Switch to Protected Mode Success~!!', 0

times 512 - ($ - $$) db 0x00
