[BITS 64]

SECTION .text

global kInPortByte, kOutPortByte, kInPortWord, kOutPortWord
global kLoadGDTR, kLoadTR, kLoadIDTR
global kEnableInterrupt, kDisableInterrupt, kReadRFLAGS
global kReadTSC
global kSwitchContext
global kHlt
global kTestAndSet
global kInitializeFPU, kSaveFPUContext, kLoadFPUContext, kSetTS, kClearTS

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

; PARAM  : WORD wPort(RDI)
; RETURN : WORD wData(RAX)
kInPortWord:
	push rdx

	mov rdx, rdi ; wPort
	mov rax, 0   ; wData(초기화)
	; DX에 저장된 포트 번호(포트 I/O 어드레스)에서 2Byte를 읽어, AX에 저장.(AX은 함수의 반환값으로 사용됨)
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
    ; DX에 저장된 포트 번호(포트 I/O 어드레스)에 AX에 저장된 값(2Byte)을 씀.
	out dx, ax

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

; PARAM  : void
; RETURN : QWORD qwData(RAX)
kReadTSC:
	push rdx

	; Read Time Stamp Counter: 타임 스탬프 카운터 레지스터(64비트)을 읽어서, 상위 32비트는 RDX에, 하위 32비트는 RAX에 저장
	rdtsc

	; RAX = RAX | (RDX << 32) : RAX는 함수의 반환값으로 사용됨
	shl rdx, 32
	or rax, rdx

	pop rdx
	ret

; 콘텍스트 저장
%macro KSAVECONTEXT 0
	; 콘텍스트 저장(범용 레지스터 15개 + 세그먼트 셀렉터 4개 = 19개)
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

	mov ax, ds ; DS, ES는 스택에 직접 push할 수 없으므로 RAX를 이용해서 push함
	push rax
	mov ax, es
	push rax
	push fs
	push gs
%endmacro

; 콘텍스트 복원
%macro KLOADCONTEXT 0
	; 콘텍스트 복원(범용 레지스터 15개 + 세그먼트 셀렉터 4개 = 19개)
	pop gs
	pop fs
	pop rax ; DS, ES는 스택에서 직접  pop할 수 없으므로 RAX를 이용해서 pop함
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

; PARAM  : void
; RETURN : void
kHlt:
	; 프로세서를 대기 상태로 전환함(쉬게 함)
	hlt
	hlt
	ret

; PARAM  : volatile BYTE* pbDest(RDI), BYTE bCmp(RSI), BYTE bSrc(RDX)
; RETURN : BOOL bRet(RAX)
; 함수 설명    : 테스트(비교)와 설정을 하나의 명령어(Atomic Operation, 원자적 연산)로 처리 [AX==bCmp, A==*pbDest, B==bSrc 에 해당]
;          -> bCmp == *pbDest 인 경우, *pbDest에  bSrc를 설정하고, TRUE(1)을 리턴함
;          -> bCmp != *pbDest 인 경우, FALSE(0)을 리턴함
kTestAndSet:
	; 1. lock
	;    -> 어셈블리어 코드에서 전치사로 사용되며, 뒤에 오는 명령어를 수행하는 동안 시스템 버스를 잠그고 다른 프로세서나 코어가 메모리에 접근할 수 없게 함
	; 2. cmpxchg A, B
	;    -> AX == A 인 경우, mov RFLAGS.ZF, 1 하고, mov A, B 함
	;    -> AX != A 인 경우, mov RFLAGS.ZF, 0 하고, mov AX, A 함
	mov rax, rsi
	lock cmpxchg byte [rdi], dl
	je .SUCCESS ; RFLAGS.ZF == 1 인 경우, .SUCCESS로 이동

.NOTSAME:
	mov rax, 0x00 ; FALSE(0)을 리턴
	ret

.SUCCESS:
	mov rax, 0x01 ; TRUE(1)을 리턴
	ret

; PARAM  : void
; RETURN : void
kInitializeFPU:
	finit ; FPU 초기화
	ret

; PARAM  : void* pvFPUContext(RDI)
; RETURN : void
kSaveFPUContext:
	fxsave [rdi] ; pvFPUContext 메모리 어드레스에 FPU 레지스터(512 byte)를 저장
	ret

; PARAM  : void* pvFPUContext(RDI)
; RETURN : void
kLoadFPUContext:
	fxrstor [rdi] ; pvFPUContext 메모리 어드레스에 저장된 FPU 레지스터(512 byte)를 복원
	ret

; PARAM  : void
; RETURN : void
kSetTS:
	push rax

	; CR0.TS(비트 3)=1 로 설정하여, 태스크 전환(Task Switched)시 7번 예외(#NM, Device Not Available)가 발생하도록 함
	mov rax, cr0
	or rax, 0x08
	mov cr0, rax

	pop rax
	ret

; PARAM  : void
; RETURN : void
kClearTS:
	; CR0.TS(비트 3)=0 으로 설정
	clts
	ret
