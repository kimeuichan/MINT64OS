[ORG 0x00]		; 코드의 시작 어드레스 0x00으로 설정
[BITS 16] 		; 이하의 코드는 16비트 코드로 설정

SECTION .text 	; text 섹션(세그먼트)을 정의

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;	코드 영역
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

START:
	mov ax, 0x1000		; 보호 모드 엔트리 포인트의 시작 어드레스(0x10000)를 세그먼트 레지스터 값으로 변환
	mov ds, ax 			; DS 세그먼트 레지스터에 설정
	mov es, ax			; es 세그먼트 레지스터에 설정

	cli 				; 인터럽트가 발생하지 못하도록 설정
	lgdt [GDTR] 			; GDTR 자료 구조를 프로세서에 설정하여 GDT 테이블 로드

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;	보호 모드로 진입
; 	Disable Paging, Disable Cache, Internal FPU, Disable Align Check.
;	eanble ProtectedMode
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	mov eax, 0x4000003B 	; PG=0, CD=1, NW=0, AM=0, WP=0, NE=1, TS=1, EM=0, MP=0, PE=1
	mov cr0, eax 			; CR0 컨트롤 레지스터에 위에서 저장한 플래그를 설정하여 보호 모드로 전환

	; 커널 코드 세그먼트를 0x00 기준 하는 것으로 교체하고 EIP 값을 0x00 기준으로 설정
	; CS 세그먼트 셀럭터 : EIP
	jmp dword 0x08: (PROTECTEDMODE - $$ + 0x10000)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;	보호 모드로 진입
;	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

[BITS 32] 				; 이하의 코드는 32비트 코드로 설정
PROTECTEDMODE:			
	mov ax, 0x10		; 보호 모드 커널용 데이터 세그먼트 디스크립터를 ax
	mov ds, ax 			; DS 세그먼트 셀렉터에 설정
	mov es, ax 			; ES 세그먼트 셀렉터에 설정
	mov fs, ax 			; FS 세그먼트 셀렉터에 설정
	mov gs, ax 			; GS 세그먼트 셀렉터에 설정

	; 스택을 0x00000000~0x0000FFFF 영역에 64KB 크기로 생성
	mov ss, ax 			; SS 세그먼트 셀렉터에 설정
	mov esp, 0xFFFE 	; ESP 레지스터의 어드레스를 0xFFFE로 설정
	mov ebp, 0xFFFE 	; EBP 레지스터의 어드레스를 0xFFFE로 설정

	; 화면에 보호 모드로 전환되었다는 메시지를 찍는다.
	push (SWITCHSUCCESSMESSAGE - $$ + 0x10000) 	; 출력할 메시지의 어드레스를 스택에 삽입
	push 2 				; 화면 Y 좌표를 스택에 삽입
	push 0 				; 화면 X 좌표를 스택에 삽입
	call PRINTMESSAGE 	; PRITMESSAGE 함수 실행
	add esp, 12 		; 삽입한 파라미터 제거

	jmp dword 0x0B: 0x10200 			; C 언어 커널이 존재하는 0x10200 어드레스로 이동하여 C언어 커널 수행

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;	함수 코드 영역
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; 메시지를 출력하는 함수
; 	스택에 x좌표, y좌표, 문자열
PRINTMESSAGE:
	push ebp 			; 베이스 포인터 레지스터(bp)를 스택에 삽입
	mov ebp, esp 		; 베이스 포인터 레지스터에 스택 포인터 레지스터의 값을 설정
	push esi 			; 함수에 임시로 사용하는 레지스터로 함수의 마지막 부분에서 스택에 삽입된 값을 꺼내 원래값으로 복원
	push edi
	push eax
	push ecx
	push edx

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; X,Y 좌표로 비디오 메모리의 어드레스를 계산함
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; Y 좌표를 이용해서 먼저 라인 어드레스를 구함
	mov eax, dword[ebp+12] 		; 파라미터 2(화면 좌표 y)를 eax에 설정
	mov esi, 160 				; 한 라인의 바이트 수 (2*80)를 esi 레지스터에 설정
	mul esi 					; eax 레지스터와 esi 레지스터를 곱하여 y 레지스터 계산
	mov edi, eax 				; 계산된 화면 y 어드레스를 edi레지스터에 설정

	; X 좌표를 이용해서 2를 곱한후 최종 어드레스를 구함
	mov eax, dword[ebp+8] 		; 파라미터 1(화면 좌표 x)를 eax에 설정
	mov esi, 2 					; 한 문자를 나타내는 바이트 수(2)를 esi 레지스터에 설정
	mul esi 					; 화면 y 어드레스와  계산된 x 어드레스를 더해서 실제 비디오 메모리 어드레스를 계산
	add edi, eax 				; 실제 비디오 메모리 어드레스를 계산

	; 출력할 문자열의 어드레스
	mov esi, dword[ebp+16] 

.MESSAGELOOP:
	mov cl, byte[esi] 			; esi 레지스터가 가리키는 문자열 위치에서 한 문자를 cl 레지스터에 복사
	 							; cl 레지스터는 ecx 레지스터의 하위 1바이트를 의미 문자열은 1바이트면 충분하므로 ecx 레지스터의 하위 1바이트만 사용
	cmp cl, 0
	je .MESSAGEEND 				; 복사한 문자의 값이 0이면 문자열이 종료 의미하므로 .MESSAGEEND로 이동

	mov byte [edi+0xB8000], cl 	; 0이 아니라면 비디오 메모리 어드레스 0xB8000 + EDI 문자를 출력

	add esi, 1 					; ESI 레지스터에 1을 더하여 다음 문자열로 이동
	add edi, 2 					; EDI 레지스터에 2를 더하여 비디오 메모리의 다음 문자 위치로 이동

	jmp .MESSAGELOOP

.MESSAGEEND:
	pop edx
	pop ecx
	pop eax
	pop edi
	pop esi
	pop ebp
	ret 						; 함수 호출한 다음 코드의 위치로 복귀

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;	데이터 영역
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; 아래의 데이터들을 8바이트에 맞춰 정렬하기 위해 추가
align 8, db 0

; GDTR의 끝을 8byte 로 정렬하기 위해 추가
dw 0x0000
; GDTR 자료 구조 정의
GDTR:
	dw GDTEND - GDT - 1 		; 아래에 위치하는 GDT 테이블의 전체 크기
	dd (GDT - $$ + 0x10000) 	; 아래에 위치하는 GDT 테이블의 시작 어드레스

; GDT 테이블 정의
GDT:
	; NULL 디스크립터, 반드시 0으로 초기화 해야함
	NULLDescriptor:
		dw 0x0000
		dw 0x0000
		db 0x00
		db 0x00
		db 0x00
		db 0x00
	; 보호 모드 커널용 코드 세그먼트 디스크립터
	CODEDESCRIPTOR:
		dw 0xFFFF 		; Limit[15:0]
		dw 0x0000		; Base [15:0]
		db 0x00 		; Base [23:16]
		db 0x9a 		; p=1, dpl=0, code segment, excute/read
		db 0xcf 		; g=1, d=1, l=0, limit[19:16]
		db 0x00 		; base[31:24]

	DATADESCRIPTOR:
		dw 0xFFFF 		; limit[15:0]
		dw 0x0000 		; base[15:0]
		db 0x00			; base[21:16]
		db 0x92 		; p=1, dpl=0, data segment, read/write
		db 0xcf 		; g=1, d=1, l=0, limit[19:16]
		db 0x00 		; base[31:24]

GDTEND:

; 보호 모드로 전환되었다는 메시지
SWITCHSUCCESSMESSAGE: db 'Switch To Protected Mode Success~!!!', 0

times 512 - ($ - $$) db 0x00 		; 512 바이트를 맞추기 위해 남은 부분을 0으로 채움




