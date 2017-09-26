[BITS 64]

SECTION .text

extern Main
; APIC ID 레지스터의 어드레스와 깨어난 코어의 개수
extern g_qwAPICIDAddress, g_iWakeUpApplicationProcessorCount

START:
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	mov rsp, 0x6FFFF8
	mov rbp, 0x6FFFF8

	; 부트로더 영역의 bsp 플래그 확인하여 bsp 이면 바로 main 호출
	cmp byte[0x7c09], 0x01
	je .BOOTSTRAPPROCESSORSTARTPONT

	; AP 만 실행하는 코드
	; 스택의 꼭대기는 APIC ID를 이용해서 0x700000 이하로 이동
	; 최대 16개 코어까지 지원 가능하므로 스택 영역인 1M를 16으로 나눈 값인
	; 64kb(0x10000) 만큼씩 아래로 이동하면서 설정
	; 꼭대기에서 APIC ID 만큼 뺀 값을 스택으로 사용

	mov rax, 0
	mov rbx, qword[g_qwAPICIDAddress]
	mov eax, dword [rbx]
	shr rax, 24 			; 비트 24~31 존재하는 APIC ID 값

	mov rbx, 0x10000 		
	mul rbx					; 스택 꼭대기에서 뺄 값을 구함용

	; 꼭대기에서 APIC ID 만큼 뺀 값을 스택으로 사용
	sub rsp, rax
	sub rbp, rax

	; 깨어난 AP 개수 1증가, LOCK 명령어를 사용하여
	; 변수에 배타적 접근이 가능하도록 함
	lock inc dword[g_iWakeUpApplicationProcessorCount]

.BOOTSTRAPPROCESSORSTARTPONT:
	call Main
	jmp $
