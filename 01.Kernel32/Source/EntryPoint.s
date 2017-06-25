[ORG 0x00]
[BITS 16]

SECTION .text

START:
	mov ax, 0x1000 ; ��ȣ��� Ŀ���� ��Ʈ�� ����Ʈ �޸� ��巹��(0x10000)
	mov ds, ax
	mov es, ax

	; A20 ����Ʈ Ȱ��ȭ
	mov ax, 0x2401        ; ��� ��ȣ(0x2401:A20 ����Ʈ Ȱ��ȭ)
	int 0x15              ; ���ͷ�Ʈ ���� ���̺� �ε���(0x15:BIOS Service->System Service)
	jc .A20_GATE_ERROR    ; ���� �߻��� ó��
	jmp .A20_GATE_SUCCESS ; ������ ó��

.A20_GATE_ERROR:
	in al, 0x92  ; �ý��� ��Ʈ�� ��Ʈ(0x92)���� 1byte�� �о� AL�� ����
	or al, 0x02  ; A20 ����Ʈ ��Ʈ(��Ʈ 1)�� 1�� ����
	and al, 0xFE ; �ý��� ���� ��Ʈ(��Ʈ 0)�� 0���� ����
	out 0x92, al ; �ý��� ��Ʈ�� ��Ʈ(0x92)�� ����� ���� ����

.A20_GATE_SUCCESS:
	; ��ȣ��� ��ȯ
	cli                 ; ���ͷ�Ʈ�� �߻����� ���ϵ��� ����
	lgdt [GDTR]         ; GDTR �ڷᱸ���� ���μ����� �����Ͽ� GDT ���̺� �ε�
	mov eax, 0x4000003B ; CR0 ����� ��������(PG=0, CD=1, NW=0, AM=0, WP=0, NE=1, ET=1, TS=1, EM=0, MP=1, PE=1)
	mov cr0, eax        ; �������� CR0�� ����
	jmp dword 0x18:(PROTECT_MODE - $$ + 0x10000) ; CS ���׸�Ʈ �����Ϳ� ��ȣ���� �ڵ� ���׸�Ʈ ��Ʈ���͸� �����ϰ�, PROTECT_MODE�� �޸� ��巹���� �̵�

[BITS 32]
PROTECT_MODE:
	mov ax, 0x20
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	mov esp, 0xFFFE
	mov ebp, 0xFFFE

	push (SWITCH_SUCCESS_MESSAGE - $$ + 0x10000)
	push 2
	push 0
	call PRINT_MESSAGE
	add esp, 12

	jmp dword 0x18:0x10200 ; CS ���׸�Ʈ �����Ϳ� ��ȣ���� �ڵ� ���׸�Ʈ ��Ʈ���͸� �����ϰ�, C��� ��Ʈ�� ����Ʈ �Լ��� �޸� ��巹��(0x10200)�� �̵�

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

align 8, db 0 ; �Ʒ��� �����͸� 8byte ũ��� ����

dw 0x0000 ; GDTR�� 8byte ũ��� �����ϱ� ���Ͽ� �߰�

GDTR:
	dw GDT_END - GDT - 1    ; GDT Size(2byte)
	dd (GDT - $$ + 0x10000) ; GDT BaseAddress(4byte)

GDT:
	NULL_DESCRIPTOR: ; �� ���׸�Ʈ ��ũ����(8byte)
		dw 0x0000
		dw 0x0000
		db 0x00
		db 0x00
		db 0x00
		db 0x00

	IA32E_CODE_DESCRIPTOR: ; IA-32e ���� �ڵ� ���׸�Ʈ ��ũ����(8byte)
		dw 0xFFFF    ; Limit=0xFFFF
		dw 0x0000    ; BaseAddress=0x0000
		db 0x00      ; BaseAddress=0x00
		db 0x9A      ; P=1, DPL=00, S=1, Type=0xA:CodeSegment(����/�б�)
		db 0xAF      ; G=1, D/B=0, L=1, AVL=0, Limit=0xF
		db 0x00      ; BaseAddress=0x00

	IA32E_DATA_DESCRIPTOR: ; IA-32e ����  ������ ���׸�Ʈ ��ũ����(8byte)
		dw 0xFFFF    ; Limit=0xFFFF
		dw 0x0000    ; BaseAddress=0x0000
		db 0x00      ; BaseAddress=0x00
		db 0x92      ; P=1, DPL=00, S=1, Type=0x2:DataSegment(�б�/����)
		db 0xAF      ; G=1, D/B=0, L=1, AVL=0, Limit=0xF
		db 0x00      ; BaseAddress=0x00

	PROTECT_CODE_DESCRIPTOR: ; ��ȣ ���� �ڵ� ���׸�Ʈ ��ũ����(8byte)
		dw 0xFFFF    ; Limit=0xFFFF
		dw 0x0000    ; BaseAddress=0x0000
		db 0x00      ; BaseAddress=0x00
		db 0x9A      ; P=1, DPL=00, S=1, Type=0xA:CodeSegment(����/�б�)
		db 0xCF      ; G=1, D/B=1, L=0, AVL=0, Limit=0xF
		db 0x00      ; BaseAddress=0x00

	PROTECT_DATA_DESCRIPTOR: ; ��ȣ ���� ������ ���׸�Ʈ ��ũ����(8byte)
		dw 0xFFFF    ; Limit=0xFFFF
		dw 0x0000    ; BaseAddress=0x0000
		db 0x00      ; BaseAddress=0x00
		db 0x92      ; P=1, DPL=00, S=1, Type=0x2:DataSegment(�б�/����)
		db 0xCF      ; G=1, D/B=1, L=0, AVL=0, Limit=0xF
		db 0x00      ; BaseAddress=0x00

GDT_END:

SWITCH_SUCCESS_MESSAGE: db 'Switch to Protected Mode Success~!!', 0

times 512 - ($ - $$) db 0x00
