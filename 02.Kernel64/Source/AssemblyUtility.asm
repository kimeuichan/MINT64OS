[BITS 64]

SECTION .text

global kInPortByte, kOutPortByte
global kLoadGDTR, kLoadTR, kLoadIDTR
global kEnableInterrupt, kDisableInterrupt, kReadRFLAGS

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
