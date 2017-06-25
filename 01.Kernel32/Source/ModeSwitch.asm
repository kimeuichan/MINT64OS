[BITS 32]

global kReadCPUID, kSwitchAndExecute64bitKernel

SECTION .text

; PARAM  : DWORD dwEAX, DWORD* pdwEAX, DWORD* pdwEBX, DWORD* pdwECX, DWORD* pdwEDX
; RETURN : void
kReadCPUID:
	push ebp
	mov ebp, esp
	push eax
	push ebx
	push ecx
	push edx
	push esi

	; EAX=0x00000000:기본 CPUID 정보 조회        (cpuid 실행후, EBX-EDX-ECX에 12byte의 프로세서 제조사 이름이 저장됨)
	; EAX=0x80000001:확장 기능 CPUID 정보 조회 (cpuid 실행후, EDX의 비트 29에 64비트 모드 지원 여부가 저장됨 )
	mov eax, dword [ebp + 8] ; dwEAX
	cpuid

	mov esi, dword [ebp + 12] ; pdwEAX
	mov dword [esi], eax;

	mov esi, dword [ebp + 16] ; pdwEBX
	mov dword [esi], ebx;

	mov esi, dword [ebp + 20] ; pdwECX
	mov dword [esi], ecx;

	mov esi, dword [ebp + 24] ; pdwEDX
	mov dword [esi], edx;

	pop esi
	pop edx
	pop ecx
	pop ebx
	pop eax
	pop ebp
	ret

; PARAM  : void
; RETURN : void
kSwitchAndExecute64bitKernel:

	; CR4 컨트롤 레지스터의 PAE(비트 5)=1 로 설정
	mov eax, cr4
	or eax, 0x20
	mov cr4, eax

	; CR3 컨트롤 레지스터에 PML4 테이블의 기준 주소(0x100000, 1MB)를 설정
	mov eax, 0x100000
	mov cr3, eax

	; IA32_EFER MSR 레지스터(0xC0000080)의 LME(비트 8)=1 로 설정
	; rdmsr, wrmsr에서 ECX:레지스터 어드레스, EDX:레지스터 값의 상위 32비트, EAX:레지스터 값의 하위 32비트를 파라미터로 사용
	mov ecx, 0xC0000080
	rdmsr
	or eax, 0x0100
	wrmsr

	; CR0 컨트롤 레지스터의 PG(비트 31)=1, CD(비트 30)=0, NW(비트 29)=0:???Write Back 정책을 사용 할 거니까, 1이 맞는 것 같은데...
	mov eax, cr0
	or eax, 0xE0000000
	xor eax, 0x60000000
	mov cr0, eax

	jmp 0x08:0x200000 ; CS 세그먼트 셀렉터에 IA-32e 모드용 코드 세그먼트 디스크립터를 설정하고, IA-32e 모드 커널의 메모리 어드레스(0x200000, 2MB)로 이동

	jmp $ ; 여기는 실행되지 않음
