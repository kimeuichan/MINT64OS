[BITS 64]

SECTION .text

global kInPortByte, kOutPortByte
global kLoadGDTR, kLoadTR, kLoadIDTR
global kEnableInterrupt, kDisableInterrupt, kReadRFLAGS

; **********[C언어에서 어셈블리 언어 함수 호출 규약 : IA-32e 모드]**********
; 파라미터 순서     정수 타입        실수 타입
;     1     RDI(1)   XMM0(11.1)
;     2     RSI(2)   XMM1(12.2)
;     3     RDX(3)   XMM2(13.3)
;     4     RCX(4)   XMM3(14.4)
;     5     R8(5)    XMM4(15.5)
;     6     R9(6)    XMM5(16.6)
;     7     스택(7)    XMM6(17.7)
;     8     스택(8)    XMM7(18.8)
;     9     스택(9)    스택(19.9)
;     10    스택(10)   스택(20.0)
;     ...   ...      ...
;
; 반환값 순서        정수 타입        실수 타입
;     -     RAX(1)   XMM0(1.1)
;     -     RDX      XMM1
; ******************************************************

; PARAM  : WORD wPort(RDI)
; RETURN : BYTE bData(RAX)
kInPortByte:
	push rdx

	mov rdx, rdi ; wPort
	mov rax, 0   ; bData(초기화)
	; DX에 저장된 포트 번호(포트 I/O 어드레스)에서 1Byte를 읽어, AL에 저장.(AL은 함수의 반환값으로 사용됨)
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
    ; DX에 저장된 포트 번호(포트 I/O 어드레스)에 AL에 저장된 값(1Byte)을 씀.
	out dx, al

	pop rax
	pop rdx
	ret

; PARAM  : QWORD qwGDTRAddress(RDI)
; RETURN : void
kLoadGDTR:
	; GDTR 자료구조의 어드레스를 GDTR 레지스터에 설정하고, GDT 테이블을 프로세서에 로드함
	lgdt [rdi]
	ret

; PARAM  : WORD wTSSSegmentOffset(DI)
; RETURN : void
kLoadTR:
	; TSS 세그먼트 디스크립터의 오프셋을 TR 레지스터에 설정하고, TSS 세그먼트를 프로세서에 로드함
	ltr di
	ret

; PARAM  : QWORD qwIDTRAddress(RDI)
; RETURN : void
kLoadIDTR:
	; IDTR 자료구조의 어드레스를 IDTR 레지서트에 설정하고, IDT 테이블을 프로세서에 로드함
	lidt [rdi]
	ret

; PARAM  : void
; RETURN : void
kEnableInterrupt:
	sti ; 프로세서에 인터럽트를 활성화
	ret

; PARAM  : void
; RETURN : void
kDisableInterrupt:
	cli ; 프로세서에 인터럽트를 비활성화
	ret

; PARAM  : void
; RETURN : QWORD qwData(RAX)
kReadRFLAGS:
	pushfq  ; RFLAGS 레지스터의 값을 스택에 저장
	pop rax ; RFLAGS 레지스터의 값을 스택에서 꺼내서 RAX 레지스터에 저장 (RAX는 함수의 반환값으로 사용됨)
	ret
