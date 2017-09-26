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
#include "FileSystem.h"
#include "MultiProcessor.h"


// AP 를 위한 메인
void MainForApplicationProcessor(void);

// Bootstrap Processor C 언어 커널 엔트리 포인트
// 아래 함수는 C 언어 커널의 시작 부분임

void Main(void){
	int iCursorX, iCursorY;

	// 부트 로더에 bsp 플래그 읽어서 ap 이면
	// ap 용 main 으로 이동
	if(*((BYTE*)BOOTSTRAPPROCESSOR_FLAGADDRESS) == 0)
		MainForApplicationProcessor();

	// bsp 부팅 완료 했으므로, 0x7c09 에 있는 bsp 플래그를
	// 0설정하여 ap용으로 코드 경로 변경
	*((BYTE*)BOOTSTRAPPROCESSOR_FLAGADDRESS) = 0;

	// ÄÜ¼Ö ÃÊ±âÈ­
	kInitializeConsole(0, 10);

	// IA-32e ¸ðµå C¾ð¾î Ä¿³Î ½ÃÀÛ ¸Þ¼¼Áö
	kPrintf("Switch to IA-32e Mode Success~!!\n");
	kPrintf("IA-32e Mode C Language Kernel Start.........[Pass]\n");
	kPrintf("Console Initialize..........................[Pass]\n");

	// GDT/TSS »ý¼º ¹× GDT ·Îµå
	kGetCursor(&iCursorX, &iCursorY);
	kPrintf("GDT/TSS Initialize and GDT Load.............[    ]");
	kInitializeGDTTableAndTSS();
	kLoadGDTR(GDTR_STARTADDRESS);
	kSetCursor(45, iCursorY++);
	kPrintf("Pass\n");

	// TSS ·Îµå
	kPrintf("TSS Load....................................[    ]");
	kLoadTR(GDT_TSSSEGMENT);
	kSetCursor(45, iCursorY++);
	kPrintf("Pass\n");

	// IDT »ý¼º ¹× ·Îµå
	kPrintf("IDT Initialize and Load.....................[    ]");
	kInitializeIDTTable();
	kLoadIDTR(IDTR_STARTADDRESS);
	kSetCursor(45, iCursorY++);
	kPrintf("Pass\n");

	// ÃÑ RAM Å©±â Ã¼Å©
	kPrintf("Total RAM Size Check........................[    ]");
	kCheckTotalRAMSize();
	kSetCursor(45, iCursorY++);
	kPrintf("Pass], Size = %d MB\n", kGetTotalRAMSize());

	// TCB Ç® ¹× ½ºÄÉÁÙ·¯ ÃÊ±âÈ­
	kPrintf("TCB Pool and Scheduler Initialize...........[Pass]\n");
	iCursorY++;
	kInitializeScheduler();

	// µ¿Àû ¸Þ¸ð¸® ÃÊ±âÈ­
	kPrintf("Dynamic Memory Initialize...................[Pass]\n");
	iCursorY++;
	kInitializeDynamicMemory();

	// PIT ÃÊ±âÈ­
	kInitializePIT(MSTOCOUNT(1), TRUE); // 1ms´ç ÇÑ ¹ø¾¿(ÁÖ±âÀûÀ¸·Î) Å¸ÀÌ¸Ó ÀÎÅÍ·´Æ®°¡ ¹ß»ýÇÏµµ·Ï ¼³Á¤

	// Å° Å¥ ÃÊ±âÈ­ ¹× Å°º¸µå È°¼ºÈ­
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

	// PIC ÃÊ±âÈ­ ¹× ÀÎÅÍ·´Æ® È°¼ºÈ­
	kPrintf("PIC Initialize and Interrupt Activate.......[    ]");
	kInitializePIC();
	kMaskPICInterrupt(0);
	kEnableInterrupt();
	kSetCursor(45, iCursorY++);
	kPrintf("Pass\n");

	// ÆÄÀÏ ½Ã½ºÅÛ ÃÊ±âÈ­
	kPrintf("File System Initialize......................[    ]");
	if(kInitializeFileSystem() == TRUE){
		kSetCursor(45, iCursorY++);
		kPrintf("Pass\n");

	}else{
		kSetCursor(45, iCursorY++);
		kPrintf("Fail\n");
	}

	// 시리얼 포트 초기화
	kPrintf("Serial Port Initialize......................[Pass]\n");
	iCursorY++;
	kInitializeSerialPort();

	// À¯ÈÞ ÅÂ½ºÅ©¸¦ »ý¼ºÇÏ°í, ÄÜ¼Ö ½©À» ½ÃÀÛ
	kCreateTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD | TASK_FLAGS_SYSTEM | TASK_FLAGS_IDLE, 0, 0, (QWORD)kIdleTask);
	kStartConsoleShell();
}

// AP 용 Main
// 대부분의 자료구조는 bsp 가 생성했으므로 코어에 설정 하는 작업만 함
void MainForApplicationProcessor(void){
	QWORD qwTickCount;

	// GDT 테이블을 설정
	kLoadGDTR(GDTR_STARTADDRESS);

	// TSS 디스크립터를 설정. TSS 세그먼트와 디스크립터를 ap의
	// 수만큼 생성했으므로, APIC ID를 이용하여 TSS 디스크립터를 할당
	kLoadTR(GDT_TSSSEGMENT + (kGetAPICID()*sizeof(GDTENTRY16)));

	// IDT 테이블 설정
	kLoadIDTR(IDTR_STARTADDRESS);

	// 1초마다 한번씩 메시지를 출력
	qwTickCount = kGetTickCount();
	while(1){
		if(kGetTickCount()-qwTickCount > 1000){
			qwTickCount = kGetTickCount();
			kPrintf("Application Processor[APIC ID: %d] is Activated\n", kGetAPICID());
		}
	}
}
