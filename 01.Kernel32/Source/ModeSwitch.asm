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

	; EAX=0x00000000:�⺻ CPUID ���� ��ȸ        (cpuid ������, EBX-EDX-ECX�� 12byte�� ���μ��� ������ �̸��� �����)
	; EAX=0x80000001:Ȯ�� ��� CPUID ���� ��ȸ (cpuid ������, EDX�� ��Ʈ 29�� 64��Ʈ ��� ���� ���ΰ� ����� )
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

	; CR4 ��Ʈ�� ���������� PAE(��Ʈ 5)=1 �� ����
	mov eax, cr4
	or eax, 0x20
	mov cr4, eax

	; CR3 ��Ʈ�� �������Ϳ� PML4 ���̺��� ���� �ּ�(0x100000, 1MB)�� ����
	mov eax, 0x100000
	mov cr3, eax

	; IA32_EFER MSR ��������(0xC0000080)�� LME(��Ʈ 8)=1 �� ����
	; rdmsr, wrmsr���� ECX:�������� ��巹��, EDX:�������� ���� ���� 32��Ʈ, EAX:�������� ���� ���� 32��Ʈ�� �Ķ���ͷ� ���
	mov ecx, 0xC0000080
	rdmsr
	or eax, 0x0100
	wrmsr

	; CR0 ��Ʈ�� ���������� PG(��Ʈ 31)=1, CD(��Ʈ 30)=0, NW(��Ʈ 29)=0:???Write Back ��å�� ��� �� �Ŵϱ�, 1�� �´� �� ������...
	mov eax, cr0
	or eax, 0xE0000000
	xor eax, 0x60000000
	mov cr0, eax

	jmp 0x08:0x200000 ; CS ���׸�Ʈ �����Ϳ� IA-32e ���� �ڵ� ���׸�Ʈ ��ũ���͸� �����ϰ�, IA-32e ��� Ŀ���� �޸� ��巹��(0x200000, 2MB)�� �̵�

	jmp $ ; ����� ������� ����
