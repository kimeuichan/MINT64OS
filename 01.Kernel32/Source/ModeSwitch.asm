[BITS 32] 				; 이하의 코드는 32비트 코드로 설정

; c언어 안에서 호출할 수 있도록 이름을 노출(Export)
global kReadCPUID, kSwitchAndExecute64bitKernel

SECTION .text 			; text 섹션(세그먼트)을 정의

; CPUID 반환
; PARAM: DWORD dwEAX, DWORD* pdwEAX,* pdwEBX, *pdwECX, *pdwEDX

kReadCPUID:
	push ebp
	mov ebp, esp
	push eax
	push ebx
	push ecx
	push edx
	push esi


	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; EAX 레지스터 값으로 CPUID 실행
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	mov eax, dword[ebp+8] 		; 파라미터 1(dwEAX)를 EAX 레지스터에 저장
	cpuid


	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; 반환된 값을 파라미터에 저장
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	; *pdwEAX
	mov esi, dword[ebp+12]
	mov dword[esi], eax

	; *pdwEBX
	mov esi, dword[ebp+16]
	mov dword[esi], ebx

	; *pdwECX
	mov esi, dword[ebp+20]
	mov dword[esi], ecx

	; *pdwEDX
	mov esi, dword[ebp+24]
	mov dword[esi], edx

	pop esi
	pop edx
	pop ecx
	pop ebx
	pop eax
	pop ebp
	ret

; IA-32e 모드로 전환하고 64비트 커널 수행
; PARAM 없음
kSwitchAndExecute64bitKernel:

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; CR4 컨트롤 레지스터의 PAE 비트를 1로 설정
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	mov eax, cr4 		; CR4 컨트롤 레지스터의 값을 EAX 레지스터에 저장
	or eax, 0x20		; PAE 비트(비트5)를 1로 설정
	mov cr4, eax		; PAE 비트가 1로 설정된 값을 CR4 컨트롤 레지스터에 저장

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; CR3 컨트롤 레지스터에 PML4 테이블의 어드레스와 캐시 활성화
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	mov eax, 0x100000 	; EAX 레지스터에 PML4 테이블이 존재하는 0x100000을 저장
	mov cr3, eax 		; CR3 컨트롤 레지스터에 0x100000을 저장

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; IA32_EFER.LME를 1로 설정하여 IA-32e 모드 활성화
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	mov ecx, 0xc0000080	; IA32_EFER MSR 레지스터의 어드레스를 저장
	rdmsr 				; MSR 레지스터 읽기

	or eax, 0x0100 		; EAX에 저장된 IA32_EFER MSR의 하위 32비트에서 LME 비트 1로 설정

	wrmsr 				; MSR 레지스터 쓰기


	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	; CR0 컨트롤 레지스터를 NW 비트(비트29)=0, CD 비트(비트30)=0, PG비트(비트31)=1 설정
	; 하여 캐시 기능과 페이징 기능 활성화
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	mov eax, cr0 		; EAX 레지스터에 CR0 컨트롤 레지스터를 저장
	or eax, 0xe0000000 	; NW 비트 ,CD 비트, PG 비트을 모두 1로 설정
	xor eax, 0x60000000 ; NW 비트와 CD 비트를 xor 하여 0으로 설정
	mov cr0, eax 		; 설정한 값을 다시 cr0에 저장


	jmp 0x08:0x200000 	; CS 세그먼트 셀럭터를 IA-32e 모드용 코드 세그먼트 디스크립터로 교체하고
						; 0x200000 어드레스로 이동


	; 여기는 실행되지 않음
	jmp $



