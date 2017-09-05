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

; **********[C���� ����� ��� �Լ� ȣ�� �Ծ� : IA-32e ���]**********
; �Ķ���� ����     ���� Ÿ��        �Ǽ� Ÿ��
;     1     RDI(1)   XMM0(11.1)
;     2     RSI(2)   XMM1(12.2)
;     3     RDX(3)   XMM2(13.3)
;     4     RCX(4)   XMM3(14.4)
;     5     R8(5)    XMM4(15.5)
;     6     R9(6)    XMM5(16.6)
;     7     ����(7)    XMM6(17.7)
;     8     ����(8)    XMM7(18.8)
;     9     ����(9)    ����(19.9)
;     10    ����(10)   ����(20.0)
;     ...   ...      ...
;
; ��ȯ�� ����        ���� Ÿ��        �Ǽ� Ÿ��
;     -     RAX(1)   XMM0(1.1)
;     -     RDX      XMM1
; ******************************************************

; PARAM  : WORD wPort(RDI)
; RETURN : BYTE bData(RAX)
kInPortByte:
	push rdx

	mov rdx, rdi ; wPort
	mov rax, 0   ; bData(�ʱ�ȭ)
	; DX�� ����� ��Ʈ ��ȣ(��Ʈ I/O ��巹��)���� 1Byte�� �о�, AL�� ����.(AL�� �Լ��� ��ȯ������ ����)
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
    ; DX�� ����� ��Ʈ ��ȣ(��Ʈ I/O ��巹��)�� AL�� ����� ��(1Byte)�� ��.
	out dx, al

	pop rax
	pop rdx
	ret

; PARAM  : WORD wPort(RDI)
; RETURN : WORD wData(RAX)
kInPortWord:
	push rdx

	mov rdx, rdi ; wPort
	mov rax, 0   ; wData(�ʱ�ȭ)
	; DX�� ����� ��Ʈ ��ȣ(��Ʈ I/O ��巹��)���� 2Byte�� �о�, AX�� ����.(AX�� �Լ��� ��ȯ������ ����)
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
    ; DX�� ����� ��Ʈ ��ȣ(��Ʈ I/O ��巹��)�� AX�� ����� ��(2Byte)�� ��.
	out dx, ax

	pop rax
	pop rdx
	ret

; PARAM  : QWORD qwGDTRAddress(RDI)
; RETURN : void
kLoadGDTR:
	; GDTR �ڷᱸ���� ��巹���� GDTR �������Ϳ� �����ϰ�, GDT ���̺��� ���μ����� �ε���
	lgdt [rdi]
	ret

; PARAM  : WORD wTSSSegmentOffset(DI)
; RETURN : void
kLoadTR:
	; TSS ���׸�Ʈ ��ũ������ �������� TR �������Ϳ� �����ϰ�, TSS ���׸�Ʈ�� ���μ����� �ε���
	ltr di
	ret

; PARAM  : QWORD qwIDTRAddress(RDI)
; RETURN : void
kLoadIDTR:
	; IDTR �ڷᱸ���� ��巹���� IDTR ������Ʈ�� �����ϰ�, IDT ���̺��� ���μ����� �ε���
	lidt [rdi]
	ret

; PARAM  : void
; RETURN : void
kEnableInterrupt:
	sti ; ���μ����� ���ͷ�Ʈ�� Ȱ��ȭ
	ret

; PARAM  : void
; RETURN : void
kDisableInterrupt:
	cli ; ���μ����� ���ͷ�Ʈ�� ��Ȱ��ȭ
	ret

; PARAM  : void
; RETURN : QWORD qwData(RAX)
kReadRFLAGS:
	pushfq  ; RFLAGS ���������� ���� ���ÿ� ����
	pop rax ; RFLAGS ���������� ���� ���ÿ��� ������ RAX �������Ϳ� ���� (RAX�� �Լ��� ��ȯ������ ����)
	ret

; PARAM  : void
; RETURN : QWORD qwData(RAX)
kReadTSC:
	push rdx

	; Read Time Stamp Counter: Ÿ�� ������ ī���� ��������(64��Ʈ)�� �о, ���� 32��Ʈ�� RDX��, ���� 32��Ʈ�� RAX�� ����
	rdtsc

	; RAX = RAX | (RDX << 32) : RAX�� �Լ��� ��ȯ������ ����
	shl rdx, 32
	or rax, rdx

	pop rdx
	ret

; ���ؽ�Ʈ ����
%macro KSAVECONTEXT 0
	; ���ؽ�Ʈ ����(���� �������� 15�� + ���׸�Ʈ ������ 4�� = 19��)
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

	mov ax, ds ; DS, ES�� ���ÿ� ���� push�� �� �����Ƿ� RAX�� �̿��ؼ� push��
	push rax
	mov ax, es
	push rax
	push fs
	push gs
%endmacro

; ���ؽ�Ʈ ����
%macro KLOADCONTEXT 0
	; ���ؽ�Ʈ ����(���� �������� 15�� + ���׸�Ʈ ������ 4�� = 19��)
	pop gs
	pop fs
	pop rax ; DS, ES�� ���ÿ��� ����  pop�� �� �����Ƿ� RAX�� �̿��ؼ� pop��
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

	; pstCurrentContext=NULL�̸�, ���ؽ�Ʈ�� ������ �ʿ� ����
	pushfq ; �Ʒ��� cmp�� ����� RFLAGS�� ������ �ʵ��� ���ÿ� ����
	cmp rdi, 0
	je .LoadConext
	popfq

	; ***** ���� �½�ũ�� ���ؽ�Ʈ�� ���� *****
	push rax ; RAX�� �������� ���������� ����ϱ� ���� ���

	; 5�� ��������(SS, RSP, RFLAGS, CS, RIP)�� CONTEXT �ڷᱸ��(pstCurrentContext)�� ����
	mov ax, ss ; SS ����
	mov qword [rdi + (23 * 8)], rax

	mov rax, rbp ; RBP�� ����� RSP ����
	add rax, 16  ; RSP �����, RBP(push rbp)�� ���� �ּҸ� ����
	mov qword [rdi + (22 * 8)], rax

	pushfq ; RFLAGS ����
	pop rax
	mov qword [rdi + (21 * 8)], rax

	mov ax, cs ; CS ����
	mov qword [rdi + (20 * 8)], rax

	mov rax, qword [rbp + 8] ; RIP�� ���� �ּҷ� �����Ͽ�, ���� ���ؽ�Ʈ �����ÿ� kSwitchContext �Լ� ȣ�� ���� �������� �̵�
	mov qword [rdi + (19 * 8)], rax

	pop rax
	pop rbp

	; CONTEXT �ڷᱸ���� 18��(RBP)~0��(GS) �����¿� ������ 19���� �������͸� �����ϱ� ����, RSP�� 19��(RIP) ������ ��ġ�� �̵�
	add rdi, (19 * 8)
	mov rsp, rdi
	sub rdi, (19 * 8)

	; ������ 19�� �������͸� CONTEXT �ڷᱸ��(pstCurrentContext)�� ����
	KSAVECONTEXT

; ***** ���� �½�ũ�� ���ؽ�Ʈ�� ���� *****
.LoadConext:
	mov rsp, rsi

	; CONTEXT �ڷᱸ��(pstNextContext)���� 19�� �������͸� ����
	KLOADCONTEXT

	; CONTEXT �ڷᱸ��(pstNextContext)���� ������ 5�� �������͸� �����ϰ�, RIP�� ����Ű�� ��巹���� ����
	iretq

; PARAM  : void
; RETURN : void
kHlt:
	; ���μ����� ��� ���·� ��ȯ��(���� ��)
	hlt
	hlt
	ret

; PARAM  : volatile BYTE* pbDest(RDI), BYTE bCmp(RSI), BYTE bSrc(RDX)
; RETURN : BOOL bRet(RAX)
; �Լ� ����    : �׽�Ʈ(��)�� ������ �ϳ��� ��ɾ�(Atomic Operation, ������ ����)�� ó�� [AX==bCmp, A==*pbDest, B==bSrc �� �ش�]
;          -> bCmp == *pbDest �� ���, *pbDest��  bSrc�� �����ϰ�, TRUE(1)�� ������
;          -> bCmp != *pbDest �� ���, FALSE(0)�� ������
kTestAndSet:
	; 1. lock
	;    -> ������� �ڵ忡�� ��ġ��� ���Ǹ�, �ڿ� ���� ��ɾ �����ϴ� ���� �ý��� ������ ��װ� �ٸ� ���μ����� �ھ �޸𸮿� ������ �� ���� ��
	; 2. cmpxchg A, B
	;    -> AX == A �� ���, mov RFLAGS.ZF, 1 �ϰ�, mov A, B ��
	;    -> AX != A �� ���, mov RFLAGS.ZF, 0 �ϰ�, mov AX, A ��
	mov rax, rsi
	lock cmpxchg byte [rdi], dl
	je .SUCCESS ; RFLAGS.ZF == 1 �� ���, .SUCCESS�� �̵�

.NOTSAME:
	mov rax, 0x00 ; FALSE(0)�� ����
	ret

.SUCCESS:
	mov rax, 0x01 ; TRUE(1)�� ����
	ret

; PARAM  : void
; RETURN : void
kInitializeFPU:
	finit ; FPU �ʱ�ȭ
	ret

; PARAM  : void* pvFPUContext(RDI)
; RETURN : void
kSaveFPUContext:
	fxsave [rdi] ; pvFPUContext �޸� ��巹���� FPU ��������(512 byte)�� ����
	ret

; PARAM  : void* pvFPUContext(RDI)
; RETURN : void
kLoadFPUContext:
	fxrstor [rdi] ; pvFPUContext �޸� ��巹���� ����� FPU ��������(512 byte)�� ����
	ret

; PARAM  : void
; RETURN : void
kSetTS:
	push rax

	; CR0.TS(��Ʈ 3)=1 �� �����Ͽ�, �½�ũ ��ȯ(Task Switched)�� 7�� ����(#NM, Device Not Available)�� �߻��ϵ��� ��
	mov rax, cr0
	or rax, 0x08
	mov cr0, rax

	pop rax
	ret

; PARAM  : void
; RETURN : void
kClearTS:
	; CR0.TS(��Ʈ 3)=0 ���� ����
	clts
	ret
