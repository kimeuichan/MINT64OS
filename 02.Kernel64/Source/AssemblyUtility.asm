[BITS 64]

SECTION .text

global kInPortByte, kOutPortByte
global kLoadGDTR, kLoadTR, kLoadIDTR
global kEnableInterrupt, kDisableInterrupt, kReadRFLAGS
global kReadTSC
global kSwitchContext

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

; PARAM : void
; RETURN QWORD(RAX)
kReadTSC:
	push rdx

	rdtsc 			; 타임 스탬프 카운터를 읽어서 RDX:RAX 저장
	shl rdx, 32 	; rdx << 32
	or rax, rdx		; rax = rax | rdx

	pop rdx
	ret

; 콘택스트 저장
%macro KSAVECONTEXT 0
	; 콘텍스트 저장(범용 레지스터 15개 + 세그먼트 셀렉터 4개 = 19개)
	push rbp
	mov rbp, rsp
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

	; ¼¼±×¸ÕÆ® ¼³·ºÅÍ ±³Ã¼ : DS, ES, FS, GS¿¡ Ä¿³Î µ¥ÀÌÅÍ ¼¼±×¸ÕÆ® µð½ºÅ©¸³ÅÍ¸¦ ÀúÀå
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
%endmacro

%macro KLOADCONTEXT 0
	; ÄÜÅØ½ºÆ® º¹¿ø
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


; Current Context 현재 콘텍스트 저장 Next Task 콘텍스트 복구
; PARAM: Current Context, Next Context
; RETURN : NULL
kSwitchContext:
	push rbp
	mov rbp, rsp

	; pstCurrentContext=NULL이면, 콘텍스트를 저장할 필요 없음
	pushfq ; 아래의 cmp의 결과로 RFLAGS가 변하지 않도록 스택에 저장
	cmp rdi, 0
	je .LoadConext
	popfq

	; ***** 현재 태스크의 콘텍스트를 저장 *****
	push rax ; RAX를 레지스터 오프셋으로 사용하기 위해 백업

	; 5개 레지스터(SS, RSP, RFLAGS, CS, RIP)를 CONTEXT 자료구조(pstCurrentContext)에 저장
	mov ax, ss ; SS 저장
	mov qword [rdi + (23 * 8)], rax

	mov rax, rbp ; RBP에 저장된 RSP 저장
	add rax, 16  ; RSP 저장시, RBP(push rbp)와 복귀 주소를 제외
	mov qword [rdi + (22 * 8)], rax

	pushfq ; RFLAGS 저장
	pop rax
	mov qword [rdi + (21 * 8)], rax

	mov ax, cs ; CS 저장
	mov qword [rdi + (20 * 8)], rax

	mov rax, qword [rbp + 8] ; RIP를 복귀 주소로 설정하여, 다음 콘텍스트 복원시에 kSwitchContext 함수 호출 다음 라인으로 이동
	mov qword [rdi + (19 * 8)], rax

	pop rax
	pop rbp

	; CONTEXT 자료구조의 18번(RBP)~0번(GS) 오프셋에 나머지 19개의 레지스터를 저장하기 위해, RSP를 19번(RIP) 오프셋 위치로 이동
	add rdi, (19 * 8)
	mov rsp, rdi
	sub rdi, (19 * 8)

	; 나머지 19개 레지스터를 CONTEXT 자료구조(pstCurrentContext)에 저장
	KSAVECONTEXT

; ***** 다음 태스크의 콘텍스트를 복원 *****
.LoadConext:
	mov rsp, rsi

	; CONTEXT 자료구조(pstNextContext)에서 19개 레지스터를 복원
	KLOADCONTEXT

	; CONTEXT 자료구조(pstNextContext)에서 나머지 5개 레지스터를 복원하고, RIP가 가리키는 어드레스로 복귀
	iretq