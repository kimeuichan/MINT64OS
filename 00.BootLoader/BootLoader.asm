[ORG 0x00]
[BITS 16]

SECTION .text ; text 섹션(세그먼트)을 정의

jmp 0x07C0:START ; CS 세그먼트 레지스터에 0x07C0 복사하면서 START 레이블로 이동

TOTAL_SECTOR_COUNT: dw 0x02    ; 부트 로더를 제외한 MINT64 OS 이미지의 크기 최대 1152섹터(0x90000byte)까지 가능
KERNEL32_SECTOR_COUNT: dw 0x02 ; 보호 모드 커널의 총 섹터 수
BOOTSTRAPPROCESSOR: db 0x01    ; Bootstrap Processor 인지 여부
STARTGRAPHICMODE: db 0x01 	   ; 그래픽 모드로 시작하는지 여부

START:
	mov ax, 0x07C0 ; ºÎÆ®·Î´õ ¸Þ¸ð¸® ¾îµå·¹½º(0x7C00)
	mov ds, ax
	mov ax, 0xB800 ; ºñµð¿À ¸Þ¸ð¸® ¾îµå·¹½º(0xB8000)
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
	mov ax, 0x00              ; ±â´É ¹øÈ£(0x00:¸®¼Â)
	mov dl, byte [BOOT_DRIVE] ; µå¶óÀÌºê ¹øÈ£
	int 0x13                  ; ÀÎÅÍ·´Æ® º¤ÅÍ Å×ÀÌºí ÀÎµ¦½º(0x13:BIOS Service->Disk I/O Service)
	jc HANDLE_DISK_ERROR      ; ¿¹¿Ü Ã³¸®

	mov ah, 0x08               ; ±â´É ¹øÈ£(0x08:µð½ºÅ© ÆÄ¶ó¹ÌÅÍ ÀÐ±â)
	mov dl, byte [BOOT_DRIVE]  ; µå¶óÀÌºê ¹øÈ£
	int 0x13                   ; ÀÎÅÍ·´Æ® º¤ÅÍ Å×ÀÌºí ÀÎµ¦½º(0x13:BIOS Service->Disk I/O Service)
	jc HANDLE_DISK_ERROR       ; ¿¹¿Ü Ã³¸®
	mov byte [LAST_HEAD], dh   ; ¸¶Áö¸· Çìµå ¹øÈ£(DH 8ºñÆ®)
	mov al, cl                 ; -
	and al, 0x3f               ; -
	mov byte [LAST_SECTOR], al ; ¸¶Áö¸· ¼½ÅÍ ¹øÈ£(CL ÇÏÀ§ 6ºñÆ®)
	mov byte [LAST_TRACK], ch  ; ¸¶Áö¸· Æ®·¢ ¹øÈ£(CH 8ºñÆ® + CL »óÀ§ 2ºñÆ®)

	mov si, 0x1000             ; ÀÐÀº ¼½ÅÍ¸¦ ÀúÀåÇÒ ¸Þ¸ð¸® ¾îµå·¹½º(ES:BX, 0x10000)
	mov es, si
	mov bx, 0x0000
	mov di, word [TOTAL_SECTOR_COUNT]

READ_DATA:
	cmp di, 0
	je READ_END
	sub di, 1

	mov ah, 0x02                 ; ±â´É ¹øÈ£(0x02:¼½ÅÍ ÀÐ±â)
	mov al, 0x01                 ; ÀÐÀ» ¼½ÅÍ ¼ö
	mov ch, byte [TRACK_NUMBER]  ; ÀÐÀ» Æ®·¢ ¹øÈ£
	mov cl, byte [SECTOR_NUMBER] ; ÀÐÀ» ¼½ÅÍ ¹øÈ£
	mov dh, byte [HEAD_NUMBER]   ; ÀÐÀ» Çì´õ ¹øÈ£
	mov dl, byte [BOOT_DRIVE]    ; µå¶óÀÌºê ¹øÈ£
	int 0x13                     ; ÀÎÅÍ·´Æ® º¤ÅÍ Å×ÀÌºí ÀÎµ¦½º(0x13:BIOS Service->Disk I/O Service)
	jc HANDLE_DISK_ERROR         ; ¿¹¿Ü Ã³¸®

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


	; VBE 기능 번호 0x4f01 을 호출하여 그래픽 모드에 대한 모드 정보 블록을 구함
	mov ax, 0x4f01	; VBE 기능 번호 0x4f01를 ax 레지스터에 저장
	mov cx, 0x117		; 1024x768 해상도에 16비트(R(5),G(6),B(5)) 색 모드 지정
	mov bx, 0x07E0	; bx 레지스터에 0x07e0 저장
	mov es, bx		; es 세그먼트 레지스터에 bx 값을 설정하고, di 레지스터에
	mov di, 0x00	; 0x00을 설정하여 0x07e0:0000 어드레스에 모드 정보 블록을 저장

	int 0x10		; 인터럽트 발생
	cmp ax, 0x004f	; 인터럽트 서비스 수행
	jne VBEERROR
    je JUMPTOPROTECTEDMODE                  ; 0x00°ú °°´Ù¸é ¹Ù·Î º¸È£ ¸ðµå·Î ÀüÈ¯

	cmp byte [ STARTGRAPHICMODE ], 0x00     ; ±×·¡ÇÈ ¸ðµå ½ÃÀÛÇÏ´ÂÁö ¿©ºÎ¸¦ 0x00°ú ºñ±³
    
    mov ax, 0x4F02      ; VBE ±â´É ¹øÈ£ 0x4F02¸¦ AX ·¹Áö½ºÅÍ¿¡ ÀúÀå
    mov bx, 0x4117      ; 1024x768 ÇØ»óµµ¿¡ 16ºñÆ®(R(5):G(6):B(5)) »öÀ» »ç¿ëÇÏ´Â 
                        ; ¼±Çü ÇÁ·¹ÀÓ ¹öÆÛ ¸ðµå ÁöÁ¤
                        ; VBE ¸ðµå ¹øÈ£(Bit 0~8) = 0x117, 
                        ; ¹öÆÛ ¸ðµå(ºñÆ® 14) = 1(¼±Çü ÇÁ·¹ÀÓ ¹öÆÛ ¸ðµå)
    int 0x10            ; ÀÎÅÍ·´Æ® ¼­ºñ½º ¼öÇà
    cmp ax, 0x004F      ; ¿¡·¯°¡ ¹ß»ýÇß´Ù¸é VBEERROR·Î ÀÌµ¿
    jne VBEERROR    
    
	; 그래픽 모드로 전환되었다면 보호 모드 커널로 이동
	jmp JUMPTOPROTECTEDMODE

VBEERROR:
	; 예외 처리
	; 그래픽 모드 전환이 실패했다는 메시지를 출력
	push CHANGEGRAPHICMODEFAIL
	push 2
	push 0
	call PRINT_MESSAGE
	add sp, 6
	jmp $

JUMPTOPROTECTEDMODE:
	; 로딩한 가상 OS 이미지 실행
	jmp 0x1000:0x0000


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

MESSAGE1:                 db 'MINT64 OS Boot Loader Start~!!', 0
DISK_ERROR_MESSAGE:       db 'DISK Error~!!', 0
IMAGE_LOADING_MESSAGE:    db 'OS Image Loading...', 0
LOADING_COMPLETE_MESSAGE: db 'Complete~!!', 0
CHANGEGRAPHICMODEFAIL	  db 'Change Graphic Mode Fail~!!', 0

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
