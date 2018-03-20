[BITS 64]

SECTION .text

; 핸들러 (6개)
extern kCommonExceptionHandler, kCommonInterruptHandler, kKeyboardHandler, kTimerHandler, kDeviceNotAvailableHandler
extern kHDDHandler

; 예외 처리용 ISR(21개)
global kISRDivideError, kISRDebug, kISRNMI, kISRBreakPoint, kISROverflow
global kISRBoundRangeExceeded, kISRInvalidOpcode, kISRDeviceNotAvailable, kISRDoubleFault, kISRCoprocessorSegmentOverrun
global kISRInvalidTSS, kISRSegmentNotPresent, kISRStackSegmentFault, kISRGeneralProtection, kISRPageFault
global kISR15, kISRFPUError, kISRAlignmentCheck, kISRMachineCheck, kISRSIMDError
global kISRETCException

; 인터럽트 처리용 ISR(17개)
global kISRTimer, kISRKeyboard, kISRSlavePIC, kISRSerial2, kISRSerial1
global kISRParallel2, kISRFloppy, kISRParallel1, kISRRTC, kISRReserved
global kISRNotUsed1, kISRNotUsed2, kISRMouse, kISRCoprocessor, kISRHDD1
global kISRHDD2, kISRETCInterrupt

; MINT64 OS의 콘텍스트 저장과 복원 순서 (IST 스택 이용)
; 1. 프로세서 처리: SS, RSP, RFLAGS, CS, RIP, 에러 코드 (옵션)
; 2. 핸들러 처리   : RBP, RAX, RBX, RCX, RDX, RDI, RSI, R8, R9, R10, R11, R12, R13, R14, R15, DS, ES, FS, GS

; 콘텍스트 저장 및 세그먼트 셀렉터 교체
%macro KSAVECONTEXT 0
	; 콘텍스트 저장(범용 레지스터 15개 + 세그먼트 셀렉터 4개 = 19개)
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

	mov ax, ds ; DS, ES는 스택에 직접 push할 수 없으므로 RAX를 이용해서 push함
	push rax
	mov ax, es
	push rax
	push fs
	push gs

	; 세그먼트 셀렉터 교체 : DS, ES, FS, GS에 커널 데이터 세그먼트 디스크립터를 저장
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
%endmacro

; 콘텍스트 복원
%macro KLOADCONTEXT 0
	; 콘텍스트 복원(범용 레지스터 15개 + 세그먼트 셀렉터 4개 = 19개)
	pop gs
	pop fs
	pop rax ; DS, ES는 스택에서 직접  pop할 수 없으므로 RAX를 이용해서 pop함
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
; 예외 처리용 ISR(21개): #0~#19, #20~#31
;====================================================================================================
; #0 : Divide Error ISR
kISRDivideError:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 0                   ; 첫번째 파라미터에 벡터 번호를 설정
	call kCommonExceptionHandler ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #1 : Debug Exception ISR
kISRDebug:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 1                   ; 첫번째 파라미터에 벡터 번호를 설정
	call kCommonExceptionHandler ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #2 : NMI Interrupt ISR
kISRNMI:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 2                   ; 첫번째 파라미터에 벡터 번호를 설정
	call kCommonExceptionHandler ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #3 : Break Point ISR
kISRBreakPoint:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 3                   ; 첫번째 파라미터에 벡터 번호를 설정
	call kCommonExceptionHandler ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #4 : Overflow ISR
kISROverflow:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 4                   ; 첫번째 파라미터에 벡터 번호를 설정
	call kCommonExceptionHandler ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #5 : BOUND Range Exceeded ISR
kISRBoundRangeExceeded:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 5                   ; 첫번째 파라미터에 벡터 번호를 설정
	call kCommonExceptionHandler ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #6 : Invalid Opcode (Undefined Opcode) ISR
kISRInvalidOpcode:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 6                   ; 첫번째 파라미터에 벡터 번호를 설정
	call kCommonExceptionHandler ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #7 : Device Not Available (No Math Coprocessor) ISR
kISRDeviceNotAvailable:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 7                   ; 첫번째 파라미터에 벡터 번호를 설정
	call kDeviceNotAvailableHandler ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #8 : Double Fault ISR
kISRDoubleFault:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 8                   ; 첫번째 파라미터에 벡터 번호를 설정
	mov rsi, qword [rbp + 8]     ; 두번째 파라미터에 에러 코드를 설정
	call kCommonExceptionHandler ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	add rsp, 8                   ; 에러 코드를 스택에서 제거
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #9 : Coprocessor Segment Overrun (Reserved) ISR
kISRCoprocessorSegmentOverrun:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 9                   ; 첫번째 파라미터에 벡터 번호를 설정
	call kCommonExceptionHandler ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #10 : Invalid TSS ISR
kISRInvalidTSS:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 10                  ; 첫번째 파라미터에 벡터 번호를 설정
	mov rsi, qword [rbp + 8]     ; 두번째 파라미터에 에러 코드를 설정
	call kCommonExceptionHandler ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	add rsp, 8                   ; 에러 코드를 스택에서 제거
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #11 : Segment Not Present ISR
kISRSegmentNotPresent:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 11                  ; 첫번째 파라미터에 벡터 번호를 설정
	mov rsi, qword [rbp + 8]     ; 두번째 파라미터에 에러 코드를 설정
	call kCommonExceptionHandler ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	add rsp, 8                   ; 에러 코드를 스택에서 제거
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #12 : Stack-Segment Fault ISR
kISRStackSegmentFault:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 12                  ; 첫번째 파라미터에 벡터 번호를 설정
	mov rsi, qword [rbp + 8]     ; 두번째 파라미터에 에러 코드를 설정
	call kCommonExceptionHandler ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	add rsp, 8                   ; 에러 코드를 스택에서 제거
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #13 : General Protection ISR
kISRGeneralProtection:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 13                  ; 첫번째 파라미터에 벡터 번호를 설정
	mov rsi, qword [rbp + 8]     ; 두번째 파라미터에 에러 코드를 설정
	call kCommonExceptionHandler ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	add rsp, 8                   ; 에러 코드를 스택에서 제거
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #14 : Page Fault ISR
kISRPageFault:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 14                  ; 첫번째 파라미터에 벡터 번호를 설정
	mov rsi, qword [rbp + 8]     ; 두번째 파라미터에 에러 코드를 설정
	call kCommonExceptionHandler ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	add rsp, 8                   ; 에러 코드를 스택에서 제거
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #15 : Intel Reserved ISR
kISR15:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 15                  ; 첫번째 파라미터에 벡터 번호를 설정
	call kCommonExceptionHandler ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #16 : x87 FPU Floating-Point Error (Math Fault) ISR
kISRFPUError:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 16                  ; 첫번째 파라미터에 벡터 번호를 설정
	call kCommonExceptionHandler ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #17 : Alignment Check ISR
kISRAlignmentCheck:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 17                  ; 첫번째 파라미터에 벡터 번호를 설정
	mov rsi, qword [rbp + 8]     ; 두번째 파라미터에 에러 코드를 설정
	call kCommonExceptionHandler ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	add rsp, 8                   ; 에러 코드를 스택에서 제거
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #18 : Machine Check ISR
kISRMachineCheck:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 18                  ; 첫번째 파라미터에 벡터 번호를 설정
	call kCommonExceptionHandler ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #19 : SIMD Floating-Point Exception ISR
kISRSIMDError:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 19                  ; 첫번째 파라미터에 벡터 번호를 설정
	call kCommonExceptionHandler ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #20~#31 : Intel Reserved ISR
kISRETCException:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 20                  ; 첫번째 파라미터에 벡터 번호를 설정
	call kCommonExceptionHandler ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

;====================================================================================================
; 인터럽트 처리용 ISR(17개): #32~#47, #48~#99
;====================================================================================================
; #32 : Timer ISR
kISRTimer:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 32                  ; 첫번째 파라미터에 벡터 번호를 설정
	call kTimerHandler           ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #33 : PS/2 Keyboard ISR
kISRKeyboard:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 33                  ; 첫번째 파라미터에 벡터 번호를 설정
	call kKeyboardHandler        ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #34 : Slave PIC Controller ISR
kISRSlavePIC:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 34                  ; 첫번째 파라미터에 벡터 번호를 설정
	call kCommonInterruptHandler ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #35 : Serial Port 2 (COM Port 2) ISR
kISRSerial2:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 35                  ; 첫번째 파라미터에 벡터 번호를 설정
	call kCommonInterruptHandler ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #36 : Serial Port 1 (COM Port 1) ISR
kISRSerial1:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 36                  ; 첫번째 파라미터에 벡터 번호를 설정
	call kCommonInterruptHandler ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #37 : Parallel Port 2 (Print Port 2) ISR
kISRParallel2:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 37                  ; 첫번째 파라미터에 벡터 번호를 설정
	call kCommonInterruptHandler ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #38 : Floppy Disk Controller ISR
kISRFloppy:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 38                  ; 첫번째 파라미터에 벡터 번호를 설정
	call kCommonInterruptHandler ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #39 : Parallel Port 1 (Print Port 1) ISR
kISRParallel1:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 39                  ; 첫번째 파라미터에 벡터 번호를 설정
	call kCommonInterruptHandler ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #40 : RTC ISR
kISRRTC:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 40                  ; 첫번째 파라미터에 벡터 번호를 설정
	call kCommonInterruptHandler ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #41 : Reserved ISR
kISRReserved:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 41                  ; 첫번째 파라미터에 벡터 번호를 설정
	call kCommonInterruptHandler ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #42 : Not Used 1 ISR
kISRNotUsed1:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 42                  ; 첫번째 파라미터에 벡터 번호를 설정
	call kCommonInterruptHandler ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #43 : Not Used 2 ISR
kISRNotUsed2:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 43                  ; 첫번째 파라미터에 벡터 번호를 설정
	call kCommonInterruptHandler ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #44 : PS/2 Mouse ISR
kISRMouse:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 44                  ; 첫번째 파라미터에 벡터 번호를 설정
	call kCommonInterruptHandler ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #45 : Coprocessor ISR
kISRCoprocessor:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 45                  ; 첫번째 파라미터에 벡터 번호를 설정
	call kCommonInterruptHandler ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #46 : Hard Disk 1 (HDD1) ISR
kISRHDD1:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 46                  ; 첫번째 파라미터에 벡터 번호를 설정
	call kHDDHandler             ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #47 : Hard Disk 2 (HDD2) ISR
kISRHDD2:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 47                  ; 첫번째 파라미터에 벡터 번호를 설정
	call kHDDHandler             ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀

; #48~#99 : ETC Interrupt ISR
kISRETCInterrupt:
	KSAVECONTEXT                 ; 콘텍스트 저장 및 세그먼트 셀렉터 교체

	mov rdi, 48                  ; 첫번째 파라미터에 벡터 번호를 설정
	call kCommonInterruptHandler ; C언어 핸들러 함수 호출

	KLOADCONTEXT                 ; 콘텍스트 복원
	iretq                        ; 프로세서가 저장한 콘텍스트를 복원하고, 실행중이던 코드로 복귀