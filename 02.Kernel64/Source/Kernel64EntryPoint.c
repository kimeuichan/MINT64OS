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

void Main(void){
	int iCursorX, iCursorY;

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

	// À¯ÈÞ ÅÂ½ºÅ©¸¦ »ý¼ºÇÏ°í, ÄÜ¼Ö ½©À» ½ÃÀÛ
	kCreateTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD | TASK_FLAGS_SYSTEM | TASK_FLAGS_IDLE, 0, 0, (QWORD)kIdleTask);
	kStartConsoleShell();
}
