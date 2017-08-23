#include "Types.h"
#include "Keyboard.h"
#include "Descriptor.h"
#include "AssemblyUtility.h"
#include "PIC.h"
#include "Console.h"
#include "ConsoleShell.h"
#include "Task.h"
#include "PIT.h"
#include "Utility.h"
#include "DynamicMemory.h"
#include "HardDisk.h"

void Start64Kernel(void){
	int iCursorX, iCursorY;

	// 콘솔 초기화
	kInitializeConsole(0, 10);

	// IA-32e 모드 C언어 커널 시작 메세지
	kPrintf("Switch to IA-32e Mode Success~!!\n");
	kPrintf("IA-32e Mode C Language Kernel Start.........[Pass]\n");
	kPrintf("Console Initialize..........................[Pass]\n");

	// GDT/TSS 생성 및 GDT 로드
	kGetCursor(&iCursorX, &iCursorY);
	kPrintf("GDT/TSS Initialize and GDT Load.............[    ]");
	kInitializeGDTTableAndTSS();
	kLoadGDTR(GDTR_STARTADDRESS);
	kSetCursor(45, iCursorY++);
	kPrintf("Pass\n");

	// TSS 로드
	kPrintf("TSS Load....................................[    ]");
	kLoadTR(GDT_TSSSEGMENT);
	kSetCursor(45, iCursorY++);
	kPrintf("Pass\n");

	// IDT 생성 및 로드
	kPrintf("IDT Initialize and Load.....................[    ]");
	kInitializeIDTTable();
	kLoadIDTR(IDTR_STARTADDRESS);
	kSetCursor(45, iCursorY++);
	kPrintf("Pass\n");

	// 총 RAM 크기 체크
	kPrintf("Total RAM Size Check........................[    ]");
	kCheckTotalRAMSize();
	kSetCursor(45, iCursorY++);
	kPrintf("Pass], Size = %d MB\n", kGetTotalRAMSize());

	// TCB 풀 및 스케줄러 초기화
	kPrintf("TCB Pool and Scheduler Initialize...........[Pass]\n");
	iCursorY++;
	kInitializeScheduler();

	// 동적 메모리 초기화
	kPrintf( "Dynamic Memory Initialize...................[Pass]\n" );
    iCursorY++;
    kInitializeDynamicMemory();

 	// 1ms당 한 번씩(주기적으로) 타이머 인터럽트가 발생하도록 설정
	kInitializePIT(MSTOCOUNT(1), TRUE);

	// 키 큐 초기화 및 키보드 활성화
	kPrintf("Key-Queue Initialize and Keyboard Activate..[    ]");
	if(kInitializeKeyboard() == TRUE){
		kSetCursor(45, iCursorY++);
		kPrintf("Pass\n");
		kChangeKeyboardLED(FALSE, FALSE, FALSE);

	}else{
		kSetCursor(45, iCursorY++);
		kPrintf("Fail\n");
		while(1);
	}

	// PIC 초기화 및 인터럽트 활성화
	kPrintf("PIC Initialize and Interrupt Activate.......[    ]");
	kInitializePIC();
	kMaskPICInterrupt(0);
	kEnableInterrupt();
	kSetCursor(45, iCursorY++);
	kPrintf("Pass\n");

	// 하드 디스크를 초기화
	kPrintf("HDD Initialize..............................[    ]");
	if(kInitializeHDD() == TRUE){
		kSetCursor(45, iCursorY++);
		kPrintf("Pass\n");
	}
	else {
		kSetCursor(45, iCursorY++);
		kPrintf("Fail\n");
	}

	// 유휴 태스크를 시스템 스레드로 생성하고 셸을 시작 
	kCreateTask(TASK_FLAGS_LOWEST | TASK_FLAGS_IDLE | TASK_FLAGS_SYSTEM | TASK_FLAGS_THREAD, 0, 0, (QWORD)kIdleTask);
	kStartConsoleShell();
}