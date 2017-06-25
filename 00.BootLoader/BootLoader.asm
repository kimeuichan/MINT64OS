[ORG 0x00]
[BITS 16]

SECTION .text

jmp 0x07C0:START ; CS 세그먼트 레지스터에 0x07C0을 복사하고, [0x07C0:START](0x7C00+START)로 이동

TOTAL_SECTOR_COUNT: dw 0x02    ; 부트로더를 제외한 MINT64 OS 이미지의 총 섹터 수 (최대 1152섹터, 0x90000byte까지 가능)
KERNEL32_SECTOR_COUNT: dw 0x02 ; 보호 모드 커널의 섹터 수

START:
	mov ax, 0x07C0 ; 부트로더 메모리 어드레스(0x7C00)
	mov ds, ax
	mov ax, 0xB800 ; 비디오 메모리 어드레스(0xB8000)
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
	mov ax, 0x00              ; 기능 번호(0x00:리셋)
	mov dl, byte [BOOT_DRIVE] ; 드라이브 번호
	int 0x13                  ; 인터럽트 벡터 테이블 인덱스(0x13:BIOS Service->Disk I/O Service)
	jc HANDLE_DISK_ERROR      ; 예외 처리

	mov ah, 0x08               ; 기능 번호(0x08:디스크 파라미터 읽기)
	mov dl, byte [BOOT_DRIVE]  ; 드라이브 번호
	int 0x13                   ; 인터럽트 벡터 테이블 인덱스(0x13:BIOS Service->Disk I/O Service)
	jc HANDLE_DISK_ERROR       ; 예외 처리
	mov byte [LAST_HEAD], dh   ; 마지막 헤드 번호(DH 8비트)
	mov al, cl                 ; -
	and al, 0x3f               ; -
	mov byte [LAST_SECTOR], al ; 마지막 섹터 번호(CL 하위 6비트)
	mov byte [LAST_TRACK], ch  ; 마지막 트랙 번호(CH 8비트 + CL 상위 2비트)

	mov si, 0x1000             ; 읽은 섹터를 저장할 메모리 어드레스(ES:BX, 0x10000)
	mov es, si
	mov bx, 0x0000
	mov di, word [TOTAL_SECTOR_COUNT]

READ_DATA:
	cmp di, 0
	je READ_END
	sub di, 1

	mov ah, 0x02                 ; 기능 번호(0x02:섹터 읽기)
	mov al, 0x01                 ; 읽을 섹터 수
	mov ch, byte [TRACK_NUMBER]  ; 읽을 트랙 번호
	mov cl, byte [SECTOR_NUMBER] ; 읽을 섹터 번호
	mov dh, byte [HEAD_NUMBER]   ; 읽을 헤더 번호
	mov dl, byte [BOOT_DRIVE]    ; 드라이브 번호
	int 0x13                     ; 인터럽트 벡터 테이블 인덱스(0x13:BIOS Service->Disk I/O Service)
	jc HANDLE_DISK_ERROR         ; 예외 처리

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

	jmp 0x1000:0x0000 ; CS 세그먼트 레지스터에 0x1000을 복사하고, [0x1000:0x0000](0x10000)로 이동

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
