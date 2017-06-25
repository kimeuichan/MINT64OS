[ORG 0x00]
[BITS 16]

SECTION .text

START:
	mov ax, 0x1000 ; 보호모드 커널의 엔트리 포인트 메모리 어드레스(0x10000)
	mov ds, ax
	mov es, ax

	; A20 게이트 활성화
	mov ax, 0x2401        ; 기능 번호(0x2401:A20 게이트 활성화)
	int 0x15              ; 인터럽트 벡터 테이블 인덱스(0x15:BIOS Service->System Service)
	jc .A20_GATE_ERROR    ; 예외 발생시 처리
	jmp .A20_GATE_SUCCESS ; 성공시 처리

.A20_GATE_ERROR:
	in al, 0x92  ; 시스템 컨트롤 포트(0x92)에서 1byte을 읽어 AL에 저장
	or al, 0x02  ; A20 게이트 비트(비트 1)을 1로 설정
	and al, 0xFE ; 시스템 리셋 비트(비트 0)을 0으로 설정
	out 0x92, al ; 시스템 컨트롤 포트(0x92)에 변경된 값을 저장

.A20_GATE_SUCCESS:
	; 보호모드 전환
	cli                 ; 인터럽트가 발생하지 못하도록 설정
	lgdt [GDTR]         ; GDTR 자료구조를 프로세서에 설정하여 GDT 테이블 로드
	mov eax, 0x4000003B ; CR0 컨드롤 레지스터(PG=0, CD=1, NW=0, AM=0, WP=0, NE=1, ET=1, TS=1, EM=0, MP=1, PE=1)
	mov cr0, eax        ; 설정값을 CR0에 저장
	jmp dword 0x18:(PROTECT_MODE - $$ + 0x10000) ; CS 세그먼트 셀렉터에 보호모드용 코드 세그먼트 디스트립터를 설정하고, PROTECT_MODE의 메모리 어드레스로 이동

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

	jmp dword 0x18:0x10200 ; CS 세그먼트 셀렉터에 보호모드용 코드 세그먼트 디스트립터를 설정하고, C언어 엔트리 포인트 함수의 메모리 어드레스(0x10200)로 이동

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

align 8, db 0 ; 아래의 데이터를 8byte 크기로 정렬

dw 0x0000 ; GDTR를 8byte 크기로 정렬하기 위하여 추가

GDTR:
	dw GDT_END - GDT - 1    ; GDT Size(2byte)
	dd (GDT - $$ + 0x10000) ; GDT BaseAddress(4byte)

GDT:
	NULL_DESCRIPTOR: ; 널 세그먼트 디스크립터(8byte)
		dw 0x0000
		dw 0x0000
		db 0x00
		db 0x00
		db 0x00
		db 0x00

	IA32E_CODE_DESCRIPTOR: ; IA-32e 모드용 코드 세그먼트 디스크립터(8byte)
		dw 0xFFFF    ; Limit=0xFFFF
		dw 0x0000    ; BaseAddress=0x0000
		db 0x00      ; BaseAddress=0x00
		db 0x9A      ; P=1, DPL=00, S=1, Type=0xA:CodeSegment(실행/읽기)
		db 0xAF      ; G=1, D/B=0, L=1, AVL=0, Limit=0xF
		db 0x00      ; BaseAddress=0x00

	IA32E_DATA_DESCRIPTOR: ; IA-32e 모드용  데이터 세그먼트 디스크립터(8byte)
		dw 0xFFFF    ; Limit=0xFFFF
		dw 0x0000    ; BaseAddress=0x0000
		db 0x00      ; BaseAddress=0x00
		db 0x92      ; P=1, DPL=00, S=1, Type=0x2:DataSegment(읽기/쓰기)
		db 0xAF      ; G=1, D/B=0, L=1, AVL=0, Limit=0xF
		db 0x00      ; BaseAddress=0x00

	PROTECT_CODE_DESCRIPTOR: ; 보호 모드용 코드 세그먼트 디스크립터(8byte)
		dw 0xFFFF    ; Limit=0xFFFF
		dw 0x0000    ; BaseAddress=0x0000
		db 0x00      ; BaseAddress=0x00
		db 0x9A      ; P=1, DPL=00, S=1, Type=0xA:CodeSegment(실행/읽기)
		db 0xCF      ; G=1, D/B=1, L=0, AVL=0, Limit=0xF
		db 0x00      ; BaseAddress=0x00

	PROTECT_DATA_DESCRIPTOR: ; 보호 모드용 데이터 세그먼트 디스크립터(8byte)
		dw 0xFFFF    ; Limit=0xFFFF
		dw 0x0000    ; BaseAddress=0x0000
		db 0x00      ; BaseAddress=0x00
		db 0x92      ; P=1, DPL=00, S=1, Type=0x2:DataSegment(읽기/쓰기)
		db 0xCF      ; G=1, D/B=1, L=0, AVL=0, Limit=0xF
		db 0x00      ; BaseAddress=0x00

GDT_END:

SWITCH_SUCCESS_MESSAGE: db 'Switch to Protected Mode Success~!!', 0

times 512 - ($ - $$) db 0x00
