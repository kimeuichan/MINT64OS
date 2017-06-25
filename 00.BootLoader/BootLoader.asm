[ORG 0x00]
[BITS 16]

SECTION .text

jmp 0x07C0:START ; CS ���׸�Ʈ �������Ϳ� 0x07C0�� �����ϰ�, [0x07C0:START](0x7C00+START)�� �̵�

TOTAL_SECTOR_COUNT: dw 0x02    ; ��Ʈ�δ��� ������ MINT64 OS �̹����� �� ���� �� (�ִ� 1152����, 0x90000byte���� ����)
KERNEL32_SECTOR_COUNT: dw 0x02 ; ��ȣ ��� Ŀ���� ���� ��

START:
	mov ax, 0x07C0 ; ��Ʈ�δ� �޸� ��巹��(0x7C00)
	mov ds, ax
	mov ax, 0xB800 ; ���� �޸� ��巹��(0xB8000)
	mov es, ax

	mov ax, 0x0000
	mov ss, ax
	mov sp, 0xFFFE
	mov bp, 0xFFFE

	mov byte [BOOT_DRIVE], dl

	mov si, 0

.SCREEN_CLEAR_LOOP:
	mov byte [es:si], 0
	mov byte [es:si+1], 0x0A
	add si, 2
	cmp si, 80*25*2
	jl .SCREEN_CLEAR_LOOP

	push MESSAGE1
	push 0
	push 0
	call PRINT_MESSAGE
	add sp, 6

	push IMAGE_LOADING_MESSAGE
	push 1
	push 0
	call PRINT_MESSAGE
	add sp, 6

RESET_DISK:
	mov ax, 0x00              ; ��� ��ȣ(0x00:����)
	mov dl, byte [BOOT_DRIVE] ; ����̺� ��ȣ
	int 0x13                  ; ���ͷ�Ʈ ���� ���̺� �ε���(0x13:BIOS Service->Disk I/O Service)
	jc HANDLE_DISK_ERROR      ; ���� ó��

	mov ah, 0x08               ; ��� ��ȣ(0x08:��ũ �Ķ���� �б�)
	mov dl, byte [BOOT_DRIVE]  ; ����̺� ��ȣ
	int 0x13                   ; ���ͷ�Ʈ ���� ���̺� �ε���(0x13:BIOS Service->Disk I/O Service)
	jc HANDLE_DISK_ERROR       ; ���� ó��
	mov byte [LAST_HEAD], dh   ; ������ ��� ��ȣ(DH 8��Ʈ)
	mov al, cl                 ; -
	and al, 0x3f               ; -
	mov byte [LAST_SECTOR], al ; ������ ���� ��ȣ(CL ���� 6��Ʈ)
	mov byte [LAST_TRACK], ch  ; ������ Ʈ�� ��ȣ(CH 8��Ʈ + CL ���� 2��Ʈ)

	mov si, 0x1000             ; ���� ���͸� ������ �޸� ��巹��(ES:BX, 0x10000)
	mov es, si
	mov bx, 0x0000
	mov di, word [TOTAL_SECTOR_COUNT]

READ_DATA:
	cmp di, 0
	je READ_END
	sub di, 1

	mov ah, 0x02                 ; ��� ��ȣ(0x02:���� �б�)
	mov al, 0x01                 ; ���� ���� ��
	mov ch, byte [TRACK_NUMBER]  ; ���� Ʈ�� ��ȣ
	mov cl, byte [SECTOR_NUMBER] ; ���� ���� ��ȣ
	mov dh, byte [HEAD_NUMBER]   ; ���� ��� ��ȣ
	mov dl, byte [BOOT_DRIVE]    ; ����̺� ��ȣ
	int 0x13                     ; ���ͷ�Ʈ ���� ���̺� �ε���(0x13:BIOS Service->Disk I/O Service)
	jc HANDLE_DISK_ERROR         ; ���� ó��

	add si, 0x0020
	mov es, si

	mov al, byte [SECTOR_NUMBER]
	add al, 1
	mov byte [SECTOR_NUMBER], al
	cmp al, byte [LAST_SECTOR]
	jbe READ_DATA

	add byte [HEAD_NUMBER], 1
	mov byte [SECTOR_NUMBER], 0x01
	mov al, byte [LAST_HEAD]
	cmp byte [HEAD_NUMBER], al
	ja .ADD_TRACK
	jmp READ_DATA

.ADD_TRACK:
	mov byte [HEAD_NUMBER], 0x00
	add byte [TRACK_NUMBER], 1
	jmp READ_DATA

READ_END:
	push LOADING_COMPLETE_MESSAGE
	push 1
	push 20
	call PRINT_MESSAGE
	add sp, 6

	jmp 0x1000:0x0000 ; CS ���׸�Ʈ �������Ϳ� 0x1000�� �����ϰ�, [0x1000:0x0000](0x10000)�� �̵�

HANDLE_DISK_ERROR:
	push DISK_ERROR_MESSAGE
	push 1
	push 20
	call PRINT_MESSAGE
	add sp, 6

	jmp $

PRINT_MESSAGE:
	push bp
	mov bp, sp
	push es
	push si
	push di
	push ax
	push cx
	push dx

	mov ax, 0xB800
	mov es, ax

	mov ax, word [bp+6]
	mov si, 160
	mul si
	mov di, ax

	mov ax, word [bp+4]
	mov si, 2
	mul si
	add di, ax

	mov si, word [bp+8]

.MESSAGE_LOOP:
	mov cl, byte [si]
	cmp cl, 0
	je .MESSAGE_END
	mov byte [es:di], cl
	add si, 1
	add di, 2
	jmp .MESSAGE_LOOP

.MESSAGE_END:
	pop dx
	pop cx
	pop ax
	pop di
	pop si
	pop es
	pop bp
	ret

MESSAGE1:                 db 0
DISK_ERROR_MESSAGE:       db 'DISK Error~!!', 0
IMAGE_LOADING_MESSAGE:    db 0
LOADING_COMPLETE_MESSAGE: db 0

SECTOR_NUMBER: db 0x02
HEAD_NUMBER:   db 0x00
TRACK_NUMBER:  db 0x00

BOOT_DRIVE:  db 0x00
LAST_SECTOR: db 0x00
LAST_HEAD:   db 0x00
LAST_TRACK:  db 0x00

times 510 - ($ - $$) db 0x00
db 0x55
db 0xAA
