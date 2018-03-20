[BITS 64]

SECTION .text

global kInPortByte, kOutPortByte, kInPortWord, kOutPortWord
global kLoadGDTR, kLoadTR, kLoadIDTR
global kEnableInterrupt, kDisableInterrupt, kReadRFLAGS
global kReadTSC
global kSwitchContext, kHlt, kTestAndSet, kPause
global kInitializeFPU, kSaveFPUContext, kLoadFPUContext, kSetTS, kClearTS
global kEnableGlobalLocalAPIC

; **********[C¾ð¾î¿¡¼­ ¾î¼Àºí¸® ¾ð¾î ÇÔ¼ö È£Ãâ ±Ô¾à : IA-32e ¸ðµå]**********
; ÆÄ¶ó¹ÌÅÍ ¼ø¼­     Á¤¼ö Å¸ÀÔ        ½Ç¼ö Å¸ÀÔ
;     1     RDI(1)   XMM0(11.1)
;     2     RSI(2)   XMM1(12.2)
;     3     RDX(3)   XMM2(13.3)
;     4     RCX(4)   XMM3(14.4)
;     5     R8(5)    XMM4(15.5)
;     6     R9(6)    XMM5(16.6)
;     7     ½ºÅÃ(7)    XMM6(17.7)
;     8     ½ºÅÃ(8)    XMM7(18.8)
;     9     ½ºÅÃ(9)    ½ºÅÃ(19.9)
;     10    ½ºÅÃ(10)   ½ºÅÃ(20.0)
;     ...   ...      ...
;
; ¹ÝÈ¯°ª ¼ø¼­        Á¤¼ö Å¸ÀÔ        ½Ç¼ö Å¸ÀÔ
;     -     RAX(1)   XMM0(1.1)
;     -     RDX      XMM1
; ******************************************************

; PARAM  : WORD wPort(RDI)
; RETURN : BYTE bData(RAX)
kInPortByte:
	push rdx

	mov rdx, rdi ; wPort
	mov rax, 0   ; bData(ÃÊ±âÈ­)
	; DX¿¡ ÀúÀåµÈ Æ÷Æ® ¹øÈ£(Æ÷Æ® I/O ¾îµå·¹½º)¿¡¼­ 1Byte¸¦ ÀÐ¾î, AL¿¡ ÀúÀå.(ALÀº ÇÔ¼öÀÇ ¹ÝÈ¯°ªÀ¸·Î »ç¿ëµÊ)
	in al, dx

	pop rdx
	ret

; PARAM  : WORD wPort(RDI), BYTE bData(RSI)
; RETURN : void
kOutPortByte:
	push rdx
	push rax

	mov rdx, rdi ; wPort
	mov rax, rsi ; bData
    ; DX¿¡ ÀúÀåµÈ Æ÷Æ® ¹øÈ£(Æ÷Æ® I/O ¾îµå·¹½º)¿¡ AL¿¡ ÀúÀåµÈ °ª(1Byte)À» ¾¸.
	out dx, al

	pop rax
	pop rdx
	ret

; PARAM  : WORD wPort(RDI)
; RETURN : WORD wData(RAX)
kInPortWord:
	push rdx

	mov rdx, rdi ; wPort
	mov rax, 0   ; wData(ÃÊ±âÈ­)
	; DX¿¡ ÀúÀåµÈ Æ÷Æ® ¹øÈ£(Æ÷Æ® I/O ¾îµå·¹½º)¿¡¼­ 2Byte¸¦ ÀÐ¾î, AX¿¡ ÀúÀå.(AXÀº ÇÔ¼öÀÇ ¹ÝÈ¯°ªÀ¸·Î »ç¿ëµÊ)
	in ax, dx

	pop rdx
	ret

; PARAM  : WORD wPort(RDI), WORD wData(RSI)
; RETURN : void
kOutPortWord:
	push rdx
	push rax

	mov rdx, rdi ; wPort
	mov rax, rsi ; wData
    ; DX¿¡ ÀúÀåµÈ Æ÷Æ® ¹øÈ£(Æ÷Æ® I/O ¾îµå·¹½º)¿¡ AX¿¡ ÀúÀåµÈ °ª(2Byte)À» ¾¸.
	out dx, ax

	pop rax
	pop rdx
	ret

; PARAM  : QWORD qwGDTRAddress(RDI)
; RETURN : void
kLoadGDTR:
	; GDTR ÀÚ·á±¸Á¶ÀÇ ¾îµå·¹½º¸¦ GDTR ·¹Áö½ºÅÍ¿¡ ¼³Á¤ÇÏ°í, GDT Å×ÀÌºíÀ» ÇÁ·Î¼¼¼­¿¡ ·ÎµåÇÔ
	lgdt [rdi]
	ret

; PARAM  : WORD wTSSSegmentOffset(DI)
; RETURN : void
kLoadTR:
	; TSS ¼¼±×¸ÕÆ® µð½ºÅ©¸³ÅÍÀÇ ¿ÀÇÁ¼ÂÀ» TR ·¹Áö½ºÅÍ¿¡ ¼³Á¤ÇÏ°í, TSS ¼¼±×¸ÕÆ®¸¦ ÇÁ·Î¼¼¼­¿¡ ·ÎµåÇÔ
	ltr di
	ret

; PARAM  : QWORD qwIDTRAddress(RDI)
; RETURN : void
kLoadIDTR:
	; IDTR ÀÚ·á±¸Á¶ÀÇ ¾îµå·¹½º¸¦ IDTR ·¹Áö¼­Æ®¿¡ ¼³Á¤ÇÏ°í, IDT Å×ÀÌºíÀ» ÇÁ·Î¼¼¼­¿¡ ·ÎµåÇÔ
	lidt [rdi]
	ret

; PARAM  : void
; RETURN : void
kEnableInterrupt:
	sti ; ÇÁ·Î¼¼¼­¿¡ ÀÎÅÍ·´Æ®¸¦ È°¼ºÈ­
	ret

; PARAM  : void
; RETURN : void
kDisableInterrupt:
	cli ; ÇÁ·Î¼¼¼­¿¡ ÀÎÅÍ·´Æ®¸¦ ºñÈ°¼ºÈ­
	ret

; PARAM  : void
; RETURN : QWORD qwData(RAX)
kReadRFLAGS:
	pushfq  ; RFLAGS ·¹Áö½ºÅÍÀÇ °ªÀ» ½ºÅÃ¿¡ ÀúÀå
	pop rax ; RFLAGS ·¹Áö½ºÅÍÀÇ °ªÀ» ½ºÅÃ¿¡¼­ ²¨³»¼­ RAX ·¹Áö½ºÅÍ¿¡ ÀúÀå (RAX´Â ÇÔ¼öÀÇ ¹ÝÈ¯°ªÀ¸·Î »ç¿ëµÊ)
	ret

; PARAM  : void
; RETURN : QWORD qwData(RAX)
kReadTSC:
	push rdx

	; Read Time Stamp Counter: Å¸ÀÓ ½ºÅÆÇÁ Ä«¿îÅÍ ·¹Áö½ºÅÍ(64ºñÆ®)À» ÀÐ¾î¼­, »óÀ§ 32ºñÆ®´Â RDX¿¡, ÇÏÀ§ 32ºñÆ®´Â RAX¿¡ ÀúÀå
	rdtsc

	; RAX = RAX | (RDX << 32) : RAX´Â ÇÔ¼öÀÇ ¹ÝÈ¯°ªÀ¸·Î »ç¿ëµÊ
	shl rdx, 32
	or rax, rdx

	pop rdx
	ret

; ÄÜÅØ½ºÆ® ÀúÀå
%macro KSAVECONTEXT 0
	; ÄÜÅØ½ºÆ® ÀúÀå(¹ü¿ë ·¹Áö½ºÅÍ 15°³ + ¼¼±×¸ÕÆ® ¼¿·ºÅÍ 4°³ = 19°³)
	push rbp
	push rax
	push rbx
	push rcx
	push rdx
	push rdi
	push rsi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15

	mov ax, ds ; DS, ES´Â ½ºÅÃ¿¡ Á÷Á¢ pushÇÒ ¼ö ¾øÀ¸¹Ç·Î RAX¸¦ ÀÌ¿ëÇØ¼­ pushÇÔ
	push rax
	mov ax, es
	push rax
	push fs
	push gs
%endmacro

; ÄÜÅØ½ºÆ® º¹¿ø
%macro KLOADCONTEXT 0
	; ÄÜÅØ½ºÆ® º¹¿ø(¹ü¿ë ·¹Áö½ºÅÍ 15°³ + ¼¼±×¸ÕÆ® ¼¿·ºÅÍ 4°³ = 19°³)
	pop gs
	pop fs
	pop rax ; DS, ES´Â ½ºÅÃ¿¡¼­ Á÷Á¢  popÇÒ ¼ö ¾øÀ¸¹Ç·Î RAX¸¦ ÀÌ¿ëÇØ¼­ popÇÔ
	mov es, ax
	pop rax
	mov ds, ax

	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rsi
	pop rdi
	pop rdx
	pop rcx
	pop rbx
	pop rax
	pop rbp
%endmacro

; PARAM  : CONTEXT* pstCurrentContext(RDI), CONTEXT* pstNextContext(RSI)
; RETURN : void
kSwitchContext:
	push rbp
	mov rbp, rsp

	; pstCurrentContext=NULLÀÌ¸é, ÄÜÅØ½ºÆ®¸¦ ÀúÀåÇÒ ÇÊ¿ä ¾øÀ½
	pushfq ; ¾Æ·¡ÀÇ cmpÀÇ °á°ú·Î RFLAGS°¡ º¯ÇÏÁö ¾Êµµ·Ï ½ºÅÃ¿¡ ÀúÀå
	cmp rdi, 0
	je .LoadConext
	popfq

	; ***** ÇöÀç ÅÂ½ºÅ©ÀÇ ÄÜÅØ½ºÆ®¸¦ ÀúÀå *****
	push rax ; RAX¸¦ ·¹Áö½ºÅÍ ¿ÀÇÁ¼ÂÀ¸·Î »ç¿ëÇÏ±â À§ÇØ ¹é¾÷

	; 5°³ ·¹Áö½ºÅÍ(SS, RSP, RFLAGS, CS, RIP)¸¦ CONTEXT ÀÚ·á±¸Á¶(pstCurrentContext)¿¡ ÀúÀå
	mov ax, ss ; SS ÀúÀå
	mov qword [rdi + (23 * 8)], rax

	mov rax, rbp ; RBP¿¡ ÀúÀåµÈ RSP ÀúÀå
	add rax, 16  ; RSP ÀúÀå½Ã, RBP(push rbp)¿Í º¹±Í ÁÖ¼Ò¸¦ Á¦¿Ü
	mov qword [rdi + (22 * 8)], rax

	pushfq ; RFLAGS ÀúÀå
	pop rax
	mov qword [rdi + (21 * 8)], rax

	mov ax, cs ; CS ÀúÀå
	mov qword [rdi + (20 * 8)], rax

	mov rax, qword [rbp + 8] ; RIP¸¦ º¹±Í ÁÖ¼Ò·Î ¼³Á¤ÇÏ¿©, ´ÙÀ½ ÄÜÅØ½ºÆ® º¹¿ø½Ã¿¡ kSwitchContext ÇÔ¼ö È£Ãâ ´ÙÀ½ ¶óÀÎÀ¸·Î ÀÌµ¿
	mov qword [rdi + (19 * 8)], rax

	pop rax
	pop rbp

	; CONTEXT ÀÚ·á±¸Á¶ÀÇ 18¹ø(RBP)~0¹ø(GS) ¿ÀÇÁ¼Â¿¡ ³ª¸ÓÁö 19°³ÀÇ ·¹Áö½ºÅÍ¸¦ ÀúÀåÇÏ±â À§ÇØ, RSP¸¦ 19¹ø(RIP) ¿ÀÇÁ¼Â À§Ä¡·Î ÀÌµ¿
	add rdi, (19 * 8)
	mov rsp, rdi
	sub rdi, (19 * 8)

	; ³ª¸ÓÁö 19°³ ·¹Áö½ºÅÍ¸¦ CONTEXT ÀÚ·á±¸Á¶(pstCurrentContext)¿¡ ÀúÀå
	KSAVECONTEXT

; ***** ´ÙÀ½ ÅÂ½ºÅ©ÀÇ ÄÜÅØ½ºÆ®¸¦ º¹¿ø *****
.LoadConext:
	mov rsp, rsi

	; CONTEXT ÀÚ·á±¸Á¶(pstNextContext)¿¡¼­ 19°³ ·¹Áö½ºÅÍ¸¦ º¹¿ø
	KLOADCONTEXT

	; CONTEXT ÀÚ·á±¸Á¶(pstNextContext)¿¡¼­ ³ª¸ÓÁö 5°³ ·¹Áö½ºÅÍ¸¦ º¹¿øÇÏ°í, RIP°¡ °¡¸®Å°´Â ¾îµå·¹½º·Î º¹±Í
	iretq

; PARAM  : void
; RETURN : void
kHlt:
	; ÇÁ·Î¼¼¼­¸¦ ´ë±â »óÅÂ·Î ÀüÈ¯ÇÔ(½¬°Ô ÇÔ)
	hlt
	hlt
	ret

; PARAM  : volatile BYTE* pbDest(RDI), BYTE bCmp(RSI), BYTE bSrc(RDX)
; RETURN : BOOL bRet(RAX)
; ÇÔ¼ö ¼³¸í    : Å×½ºÆ®(ºñ±³)¿Í ¼³Á¤À» ÇÏ³ªÀÇ ¸í·É¾î(Atomic Operation, ¿øÀÚÀû ¿¬»ê)·Î Ã³¸® [AX==bCmp, A==*pbDest, B==bSrc ¿¡ ÇØ´ç]
;          -> bCmp == *pbDest ÀÎ °æ¿ì, *pbDest¿¡  bSrc¸¦ ¼³Á¤ÇÏ°í, TRUE(1)À» ¸®ÅÏÇÔ
;          -> bCmp != *pbDest ÀÎ °æ¿ì, FALSE(0)À» ¸®ÅÏÇÔ
kTestAndSet:
	; 1. lock
	;    -> ¾î¼Àºí¸®¾î ÄÚµå¿¡¼­ ÀüÄ¡»ç·Î »ç¿ëµÇ¸ç, µÚ¿¡ ¿À´Â ¸í·É¾î¸¦ ¼öÇàÇÏ´Â µ¿¾È ½Ã½ºÅÛ ¹ö½º¸¦ Àá±×°í ´Ù¸¥ ÇÁ·Î¼¼¼­³ª ÄÚ¾î°¡ ¸Þ¸ð¸®¿¡ Á¢±ÙÇÒ ¼ö ¾ø°Ô ÇÔ
	; 2. cmpxchg A, B
	;    -> AX == A ÀÎ °æ¿ì, mov RFLAGS.ZF, 1 ÇÏ°í, mov A, B ÇÔ
	;    -> AX != A ÀÎ °æ¿ì, mov RFLAGS.ZF, 0 ÇÏ°í, mov AX, A ÇÔ
	mov rax, rsi
	lock cmpxchg byte [rdi], dl
	je .SUCCESS ; RFLAGS.ZF == 1 ÀÎ °æ¿ì, .SUCCESS·Î ÀÌµ¿

.NOTSAME:
	mov rax, 0x00 ; FALSE(0)À» ¸®ÅÏ
	ret

.SUCCESS:
	mov rax, 0x01 ; TRUE(1)À» ¸®ÅÏ
	ret

; PARAM  : void
; RETURN : void
kInitializeFPU:
	finit ; FPU ÃÊ±âÈ­
	ret

; PARAM  : void* pvFPUContext(RDI)
; RETURN : void
kSaveFPUContext:
	fxsave [rdi] ; pvFPUContext ¸Þ¸ð¸® ¾îµå·¹½º¿¡ FPU ·¹Áö½ºÅÍ(512 byte)¸¦ ÀúÀå
	ret

; PARAM  : void* pvFPUContext(RDI)
; RETURN : void
kLoadFPUContext:
	fxrstor [rdi] ; pvFPUContext ¸Þ¸ð¸® ¾îµå·¹½º¿¡ ÀúÀåµÈ FPU ·¹Áö½ºÅÍ(512 byte)¸¦ º¹¿ø
	ret

; PARAM  : void
; RETURN : void
kSetTS:
	push rax

	; CR0.TS(ºñÆ® 3)=1 ·Î ¼³Á¤ÇÏ¿©, ÅÂ½ºÅ© ÀüÈ¯(Task Switched)½Ã 7¹ø ¿¹¿Ü(#NM, Device Not Available)°¡ ¹ß»ýÇÏµµ·Ï ÇÔ
	mov rax, cr0
	or rax, 0x08
	mov cr0, rax

	pop rax
	ret

; PARAM  : void
; RETURN : void
kClearTS:
	; CR0.TS(ºñÆ® 3)=0 À¸·Î ¼³Á¤
	clts
	ret


; IA32_APIC_BASE MSR의 전역 홀성화 필드(11)를 1로 설정하여 APIC 활성화
; PARAM : 없음
kEnableGlobalLocalAPIC:
	push rax
	push rcx
	push rdx

	; IA_32APIc Base MBr에 설정된 기존 값을 읽어서 전역 APIC 활성화
	mov rcx, 27		; IA32_APIC_BASE MSR은 레지스터 어드레스 27위치ㅔ
	rdmsr 			; MSR의 상위 32비트와 하위 32비트는 각각 edx레지스터와
					; EAX 레지스터를 사용함

					
	or eax, 0x0800
	wrmsr

	pop rdx
	pop rcx
	pop rax
	ret

; Processor 쉬게함
; PARAM : 없음
kPause:
	pause
	ret
