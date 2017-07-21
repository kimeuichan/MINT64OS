[BITS 64]

SECTION .text

; �ڵ鷯 (3��)
extern kCommonExceptionHandler, kCommonInterruptHandler, kKeyboardHandler
extern kTimerHandler, kDeviceNotAvailableHandler

; ���� ó���� ISR(21��)
global kISRDivideError, kISRDebug, kISRNMI, kISRBreakPoint, kISROverflow
global kISRBoundRangeExceeded, kISRInvalidOpcode, kISRDeviceNotAvailable, kISRDoubleFault, kISRCoprocessorSegmentOverrun
global kISRInvalidTSS, kISRSegmentNotPresent, kISRStackSegmentFault, kISRGeneralProtection, kISRPageFault
global kISR15, kISRFPUError, kISRAlignmentCheck, kISRMachineCheck, kISRSIMDError
global kISRETCException

; ���ͷ�Ʈ ó���� ISR(17��)
global kISRTimer, kISRKeyboard, kISRSlavePIC, kISRSerial2, kISRSerial1
global kISRParallel2, kISRFloppy, kISRParallel1, kISRRTC, kISRReserved
global kISRNotUsed1, kISRNotUsed2, kISRMouse, kISRCoprocessor, kISRHDD1
global kISRHDD2, kISRETCInterrupt

; MINT64 OS�� ���ؽ�Ʈ ����� ���� ���� (IST ���� �̿�)
; 1. ���μ��� ó��: SS, RSP, RFLAGS, CS, RIP, ���� �ڵ� (�ɼ�)
; 2. �ڵ鷯 ó��   : RBP, RAX, RBX, RCX, RDX, RDI, RSI, R8, R9, R10, R11, R12, R13, R14, R15, DS, ES, FS, GS

%macro KSAVECONTEXT 0
	; ���ؽ�Ʈ ����(���� �������� 15�� + ���׸�Ʈ ������ 4�� = 19��)
	push rbp
	mov rbp, rsp
	push rax
	push rbx
	push rcx
	push rdx
	push rdi
	push rsi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15

	mov ax, ds ; DS, ES�� ���ÿ� ���� push�� �� �����Ƿ� RAX�� �̿��ؼ� push��
	push rax
	mov ax, es
	push rax
	push fs
	push gs

	; ���׸�Ʈ ������ ��ü : DS, ES, FS, GS�� Ŀ�� ������ ���׸�Ʈ ��ũ���͸� ����
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
%endmacro

%macro KLOADCONTEXT 0
	; ���ؽ�Ʈ ����
	pop gs
	pop fs
	pop rax ; DS, ES�� ���ÿ��� ����  pop�� �� �����Ƿ� RAX�� �̿��ؼ� pop��
	mov es, ax
	pop rax
	mov ds, ax

	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rsi
	pop rdi
	pop rdx
	pop rcx
	pop rbx
	pop rax
	pop rbp
%endmacro

;====================================================================================================
; ���� ó���� ISR(21��): #0~#19, #20~#31
;====================================================================================================
; #0 : Divide Error ISR
kISRDivideError:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 0                   ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	call kCommonExceptionHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #1 : Debug Exception ISR
kISRDebug:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 1                   ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	call kCommonExceptionHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #2 : NMI Interrupt ISR
kISRNMI:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 2                   ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	call kCommonExceptionHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #3 : Break Point ISR
kISRBreakPoint:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 3                   ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	call kCommonExceptionHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #4 : Overflow ISR
kISROverflow:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 4                   ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	call kCommonExceptionHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #5 : BOUND Range Exceeded ISR
kISRBoundRangeExceeded:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 5                   ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	call kCommonExceptionHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #6 : Invalid Opcode (Undefined Opcode) ISR
kISRInvalidOpcode:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 6                   ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	call kCommonExceptionHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #7 : Device Not Available (No Math Coprocessor) ISR
kISRDeviceNotAvailable:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 7                   ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	call kDeviceNotAvailableHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #8 : Double Fault ISR
kISRDoubleFault:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 8                   ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	mov rsi, qword [rbp + 8]     ; �ι�° �Ķ���Ϳ� ���� �ڵ带 ����
	call kCommonExceptionHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	add rsp, 8                   ; ���� �ڵ带 ���ÿ��� ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #9 : Coprocessor Segment Overrun (Reserved) ISR
kISRCoprocessorSegmentOverrun:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 9                   ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	call kCommonExceptionHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #10 : Invalid TSS ISR
kISRInvalidTSS:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 10                  ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	mov rsi, qword [rbp + 8]     ; �ι�° �Ķ���Ϳ� ���� �ڵ带 ����
	call kCommonExceptionHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	add rsp, 8                   ; ���� �ڵ带 ���ÿ��� ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #11 : Segment Not Present ISR
kISRSegmentNotPresent:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 11                  ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	mov rsi, qword [rbp + 8]     ; �ι�° �Ķ���Ϳ� ���� �ڵ带 ����
	call kCommonExceptionHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	add rsp, 8                   ; ���� �ڵ带 ���ÿ��� ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #12 : Stack-Segment Fault ISR
kISRStackSegmentFault:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 12                  ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	mov rsi, qword [rbp + 8]     ; �ι�° �Ķ���Ϳ� ���� �ڵ带 ����
	call kCommonExceptionHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	add rsp, 8                   ; ���� �ڵ带 ���ÿ��� ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #13 : General Protection ISR
kISRGeneralProtection:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 13                  ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	mov rsi, qword [rbp + 8]     ; �ι�° �Ķ���Ϳ� ���� �ڵ带 ����
	call kCommonExceptionHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	add rsp, 8                   ; ���� �ڵ带 ���ÿ��� ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #14 : Page Fault ISR
kISRPageFault:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 14                  ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	mov rsi, qword [rbp + 8]     ; �ι�° �Ķ���Ϳ� ���� �ڵ带 ����
	call kCommonExceptionHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	add rsp, 8                   ; ���� �ڵ带 ���ÿ��� ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #15 : Intel Reserved ISR
kISR15:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 15                  ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	call kCommonExceptionHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #16 : x87 FPU Floating-Point Error (Math Fault) ISR
kISRFPUError:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 16                  ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	call kCommonExceptionHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #17 : Alignment Check ISR
kISRAlignmentCheck:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 17                  ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	mov rsi, qword [rbp + 8]     ; �ι�° �Ķ���Ϳ� ���� �ڵ带 ����
	call kCommonExceptionHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	add rsp, 8                   ; ���� �ڵ带 ���ÿ��� ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #18 : Machine Check ISR
kISRMachineCheck:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 18                  ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	call kCommonExceptionHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #19 : SIMD Floating-Point Exception ISR
kISRSIMDError:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 19                  ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	call kCommonExceptionHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #20~#31 : Intel Reserved ISR
kISRETCException:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 20                  ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	call kCommonExceptionHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

;====================================================================================================
; ���ͷ�Ʈ ó���� ISR(17��): #32~#47, #48~#99
;====================================================================================================
; #32 : Timer ISR
kISRTimer:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 32                  ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	call kTimerHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #33 : PS/2 Keyboard ISR
kISRKeyboard:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 33                  ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	call kKeyboardHandler        ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #34 : Slave PIC Controller ISR
kISRSlavePIC:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 34                  ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	call kCommonInterruptHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #35 : Serial Port 2 (COM Port 2) ISR
kISRSerial2:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 35                  ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	call kCommonInterruptHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #36 : Serial Port 1 (COM Port 1) ISR
kISRSerial1:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 36                  ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	call kCommonInterruptHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #37 : Parallel Port 2 (Print Port 2) ISR
kISRParallel2:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 37                  ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	call kCommonInterruptHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #38 : Floppy Disk Controller ISR
kISRFloppy:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 38                  ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	call kCommonInterruptHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #39 : Parallel Port 1 (Print Port 1) ISR
kISRParallel1:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 39                  ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	call kCommonInterruptHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #40 : RTC ISR
kISRRTC:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 40                  ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	call kCommonInterruptHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #41 : Reserved ISR
kISRReserved:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 41                  ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	call kCommonInterruptHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #42 : Not Used 1 ISR
kISRNotUsed1:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 42                  ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	call kCommonInterruptHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #43 : Not Used 2 ISR
kISRNotUsed2:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 43                  ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	call kCommonInterruptHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #44 : PS/2 Mouse ISR
kISRMouse:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 44                  ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	call kCommonInterruptHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #45 : Coprocessor ISR
kISRCoprocessor:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 45                  ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	call kCommonInterruptHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #46 : Hard Disk 1 (HDD1) ISR
kISRHDD1:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 46                  ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	call kCommonInterruptHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #47 : Hard Disk 2 (HDD2) ISR
kISRHDD2:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 47                  ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	call kCommonInterruptHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����

; #48~#99 : ETC Interrupt ISR
kISRETCInterrupt:
	KSAVECONTEXT                 ; ���ؽ�Ʈ ���� �� ���׸�Ʈ ������ ��ü

	mov rdi, 48                  ; ù��° �Ķ���Ϳ� ���� ��ȣ�� ����
	call kCommonInterruptHandler ; C��� �ڵ鷯 �Լ� ȣ��

	KLOADCONTEXT                 ; ���ؽ�Ʈ ����
	iretq                        ; ���μ����� ������ ���ؽ�Ʈ�� �����ϰ�, �������̴� �ڵ�� ����
