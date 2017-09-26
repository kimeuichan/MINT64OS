#include "ConsoleShell.h"
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"
#include "PIT.h"
#include "RTC.h"
#include "AssemblyUtility.h"
#include "Task.h"
#include "Synchronization.h"
#include "DynamicMemory.h"
#include "HardDisk.h"
#include "FileSystem.h"
#include "SerialPort.h"
#include "MPConfigurationTable.h"
#include "LocalAPIC.h"
#include "MultiProcessor.h"

/***** Àü¿ª º¯¼ö Á¤ÀÇ *****/
// Ä¿¸Çµå Å×ÀÌºí
static SHELLCOMMANDENTRY gs_vstCommandTable[] = {
		{"help", "Show Help", kHelp},
		{"cls", "Clear Screen", kCls},
		{"totalram", "Show Total RAM Size", kShowTotalRAMSize},
		{"strtod", "String to Decimal/Hex Convert, ex) strtod 19 0x1F", kStringToDecimalHexTest},
		{"shutdown", "Shutdown and Reboot OS", kShutdown},
		{"settimer", "Set PIT Controller Counter0, ex) settimer 1(ms) 1(periodic)", kSetTimer},
		{"wait", "Wait ms Using PIT, ex) wait 1(ms)", kWaitUsingPIT},
		{"rdtsc", "Read Time Stamp Counter", kReadTimeStampCounter},
		{"cpuspeed", "Measure Processor Speed", kMeasureProcessorSpeed},
		{"date", "Show Current Data and Time", kShowDateAndTime},
		{"createtask", "Create Task, ex) createtask 1(type) 1022(count)", kCreateTestTask},
		{"changepriority" ,"Change Task Priority, ex) changepriority 0x300000002(taskId) 0(priority)", kChangeTaskPriority},
		{"tasklist", "Show Task List", kShowTaskList},
		{"killtask", "End Task, ex) killtask 0x300000002(taskId) or all", kKillTask},
		{"cpuload", "Show Processor Load", kCPULoad},
		{"testmutex", "Test Mutex Function", kTestMutex},
		{"testthread", "Test Thread and Process Function", kTestThread},
		{"showmatrix", "Show Matrix Screen", kShowMatrix},
		{"testpie", "Test PIE Calculation", kTestPIE},
		{"dynamicmeminfo", "Show Dynamic Memory Information", kShowDynamicMemoryInformation},
		{"testseqalloc", "Test Sequential Allocation & Free", kTestSequentialAllocation},
		{"testranalloc", "Test Random Allocation & Free", kTestRandomAllocation},
		{"hddinfo", "Show HDD Information", kShowHDDInformation},
		{"readsector", "Read HDD Sector, ex) readsector 0(LBA) 10(count)", kReadSector},
		{"writesector", "Write HDD Sector, ex) writesector 0(LBA) 10(count)", kWriteSector},
		{"mounthdd", "Mount HDD", kMountHDD},
		{"formathdd", "Format HDD", kFormatHDD},
		{"filesysteminfo", "Show File System Information", kShowFileSystemInformation},
		{"createfile", "Create File, ex) createfile a.txt", kCreateFileInRootDirectory},
		{"deletefile", "Delete File, ex) deletefile a.txt", kDeleteFileInRootDirectory},
		{"dir", "Show Root Directory", kShowRootDirectory},
		{"writefile", "Write Data to File, ex) writefile a.txt", kWriteDataToFile},
		{"readfile", "Read Data from File, ex) readfile a.txt", kReadDataFromFile},
		{"testfileio", "Test File I/O Function", kTestFileIO},
		{"testperformance", "Test File Read/Write Performance", kTestPerformance},
		{"flush", "Flush File System Cache", kFlushCache},
		{"download", "Download Data form Serial, ex)download a.txt", kDownloadFile},
		{"showmpinfo", "Show MP Configuration Table Information", kShowMPConfigurationTable},
		{"startap", "Start Application Processor", kStartApplicationProcessor},
};

//====================================================================================================
// ½© ÄÚµå ÇÔ¼ö
//====================================================================================================
void kStartConsoleShell(void){
	char vcCommandBuffer[CONSOLESHELL_MAXCOMMANDBUFFERCOUNT];
	int iCommandBufferIndex = 0;
	BYTE bKey;
	int iCursorX, iCursorY;

	kPrintf(CONSOLESHELL_PROMPTMESSAGE);

	// ÄÜ¼Ö ½© ¸ÞÀÎ ·çÇÁ
	while(1){

		// Å°°¡ ¼ö½ÅµÉ ¶§±îÁö ´ë±â
		bKey = kGetCh();

		// BackspaceÅ° Ã³¸®
		if(bKey == KEY_BACKSPACE){
			if(iCommandBufferIndex > 0){
				kGetCursor(&iCursorX, &iCursorY);
				kPrintStringXY(iCursorX - 1, iCursorY, " ");
				kSetCursor(iCursorX - 1, iCursorY);
				iCommandBufferIndex--;
			}

		// EnterÅ° Ã³¸®
		}else if(bKey == KEY_ENTER){
			kPrintf("\n");

			// Ä¿¸Çµå ½ÇÇà
			if(iCommandBufferIndex > 0){
				vcCommandBuffer[iCommandBufferIndex] = '\0';
				kExecuteCommand(vcCommandBuffer);
			}

			// È­¸é ¹× Ä¿¸Çµå ¹öÆÛ ÃÊ±âÈ­
			kPrintf("%s", CONSOLESHELL_PROMPTMESSAGE);
			kMemSet(vcCommandBuffer, '\0', CONSOLESHELL_MAXCOMMANDBUFFERCOUNT);
			iCommandBufferIndex = 0;

		// ShiftÅ°, Caps LockÅ°, Num LockÅ°, Scroll LockÅ°´Â ¹«½Ã
		}else if(bKey == KEY_LSHIFT || bKey == KEY_RSHIFT || bKey == KEY_CAPSLOCK || bKey == KEY_NUMLOCK || bKey == KEY_SCROLLLOCK){
			;

		// ±× ¿Ü Å° Ã³¸®
		}else{
			if(bKey == KEY_TAB){
				bKey = ' ';
			}

			if(iCommandBufferIndex < CONSOLESHELL_MAXCOMMANDBUFFERCOUNT){
				vcCommandBuffer[iCommandBufferIndex++] = bKey;
				kPrintf("%c", bKey);
			}
		}
	}
}

void kExecuteCommand(const char* pcCommandBuffer){
	int i, iSpaceIndex;
	int iCommandBufferLength, iCommandLength;
	int iCount;

	iCommandBufferLength = kStrLen(pcCommandBuffer);

	// ½ºÆäÀÌ½º À§Ä¡(=Ä¿¸Çµå ±æÀÌ) Ãëµæ
	for(iSpaceIndex = 0; iSpaceIndex < iCommandBufferLength; iSpaceIndex++){
		if(pcCommandBuffer[iSpaceIndex] == ' '){
			break;
		}
	}

	iCount = sizeof(gs_vstCommandTable) / sizeof(SHELLCOMMANDENTRY);

	for(i = 0; i < iCount; i++){
		iCommandLength = kStrLen(gs_vstCommandTable[i].pcCommand);

		// Ä¿¸ÇµåÀÇ ±æÀÌ¿Í ³»¿ëÀÌ ÀÏÄ¡ÇÏ´Â °æ¿ì, Ä¿¸Çµå Ã³¸® ÇÔ¼ö È£Ãâ
		if((iCommandLength == iSpaceIndex) && (kMemCmp(gs_vstCommandTable[i].pcCommand, pcCommandBuffer, iSpaceIndex) == 0)){
			gs_vstCommandTable[i].pfFunction(pcCommandBuffer + iSpaceIndex + 1); // Ä¿¸Çµå Ã³¸® ÇÔ¼ö¸¦ È£ÃâÇÏ¸é¼­, ÆÄ¶ó¹ÌÅÍ ¸®½ºÆ® Àü´Þ
			break;
		}
	}

	// Ä¿¸Çµå Å×ÀÌºí¿¡ Ä¿¸Çµå°¡ ¾ø´Ù¸é, ¿¡·¯ Ãâ·Â
	if(i >= iCount){
		kPrintf("'%s' is not found.\n", pcCommandBuffer);
	}
}

void kInitializeParameter(PARAMETERLIST* pstList, const char* pcParameterBuffer){
	pstList->pcBuffer = pcParameterBuffer;
	pstList->iLength = kStrLen(pcParameterBuffer);
	pstList->iCurrentPosition = 0;
}

int kGetNextParameter(PARAMETERLIST* pstList, char* pcParameter){
	int i;
	int iLength;

	if(pstList->iCurrentPosition >= pstList->iLength){
		return 0;
	}

	// ½ºÆäÀÌ½º À§Ä¡(=ÆÄ¶ó¹ÌÅÍ ±æÀÌ) Ãëµæ
	for(i = pstList->iCurrentPosition; i < pstList->iLength; i++){
		if(pstList->pcBuffer[i] == ' '){
			break;
		}
	}

	// ÆÄ¶ó¹ÌÅÍ º¹»ç ¹× À§Ä¡ °»½Å
	kMemCpy(pcParameter, pstList->pcBuffer + pstList->iCurrentPosition, i);
	iLength = i - pstList->iCurrentPosition;
	pcParameter[iLength] = '\0';
	pstList->iCurrentPosition += iLength + 1;

	return iLength; // ÇöÀç ÆÄ¶ó¹ÌÅÍ ±æÀÌ ¹ÝÈ¯
}

//====================================================================================================
// Ä¿¸Çµå Ã³¸® ÇÔ¼ö
//====================================================================================================
static void kHelp(const char* pcParameterBuffer){
	int i;
	int iCount;
	int iCursorX, iCursorY;
	int iLength, iMaxCommandLength = 0;

	kPrintf("===============================================================================\n");
	kPrintf("                               MINT64 Shell Help                               \n");
	kPrintf("-------------------------------------------------------------------------------\n");

	iCount = sizeof(gs_vstCommandTable) / sizeof(SHELLCOMMANDENTRY);

	// °¡Àå ±ä Ä¿¸ÇµåÀÇ ±æÀÌ¸¦ °è»ê
	for(i = 0; i < iCount; i++){
		iLength = kStrLen(gs_vstCommandTable[i].pcCommand);
		if(iLength > iMaxCommandLength){
			iMaxCommandLength = iLength;
		}
	}

	// µµ¿ò¸» Ãâ·Â
	for(i = 0; i < iCount; i++){
		kPrintf("%s", gs_vstCommandTable[i].pcCommand);
		kGetCursor(&iCursorX, &iCursorY);
		kSetCursor(iMaxCommandLength, iCursorY);
		kPrintf(" - %s\n", gs_vstCommandTable[i].pcHelp);

		// µµ¿ò¸» ¸ñ·ÏÀ» 15°³ Ãâ·Â½Ã¸¶´Ù ¸ñ·ÏÀ» ´õ Ãâ·ÂÇÒÁö ¿©ºÎ¸¦ È®ÀÎ
		if((i != 0) && ((i % 15) == 0)){

			kPrintf("Press any key to continue...('q' is exit):");

			if(kGetCh() == 'q'){
				kPrintf("\n");
				break;
			}

			kPrintf("\n");
		}
	}

	kPrintf("===============================================================================\n");
}

static void kCls(const char* pcParameterBuffer){
	kClearScreen();
	kSetCursor(0, 1); // Ã¹¹øÂ° ÁÙ¿¡´Â ÀÎÅÍ·´Æ®¸¦ Ç¥½ÃÇÏ±â ¶§¹®¿¡, µÎ¹øÂ° ÁÙ·Î Ä¿¼­ ÀÌµ¿
}

static void kShowTotalRAMSize(const char* pcParameterBuffer){
	kPrintf("Total RAM Size = %d MB\n", kGetTotalRAMSize());
}

static void kStringToDecimalHexTest(const char* pcParameterBuffer){
	PARAMETERLIST stList;
	char vcParameter[100];
	int iLength;
	int iCount = 0;
	long lValue;

	// ÆÄ¶ó¹ÌÅÍ ÃÊ±âÈ­
	kInitializeParameter(&stList, pcParameterBuffer);

	while(1){

		// ÆÄ¶ó¹ÌÅÍ Ãëµæ : Decimal/Hex String
		iLength = kGetNextParameter(&stList, vcParameter);

		if(iLength == 0){
			break;
		}

		kPrintf("Param %d = '%s', Length = %d, ", iCount + 1, vcParameter, iLength);

		// ÆÄ¶ó¹ÌÅÍ°¡ 16Áø¼öÀÎ °æ¿ì
		if(kMemCmp(vcParameter, "0x", 2) == 0){
			lValue = kAToI(vcParameter + 2, 16);
			kPrintf("Hex Value = 0x%q\n", lValue); // Ãâ·Â¿¡ 0x¸¦ Ãß°¡

		// ÆÄ¶ó¹ÌÅÍ°¡ 10Áø¼öÀÎ °æ¿ì
		}else{
			lValue = kAToI(vcParameter, 10);
			kPrintf("Decimal Value = %d\n", lValue);
		}

		iCount++;
	}
}

static void kShutdown(const char* pcParameterBuffer){
	kPrintf("System Shutdown Start...\n");

	// ÆÄÀÏ ½Ã½ºÅÛ Ä³½Ã ¹öÆÛÀÇ µ¥ÀÌÅÍ¸¦ ÇÏµå µð½ºÅ©¿¡ ¾¸
	kPrintf("Flush File System Cache...\n");
	if(kFlushFileSystemCache() == TRUE){
		kPrintf("Success~!!\n");

	}else{
		kPrintf("Fail~!!\n");
	}

	// Å°º¸µå ÄÁÆ®·Ñ·¯¸¦ ÅëÇØ PC¸¦ ÀçºÎÆÃ
	kPrintf("Press any key to reboot PC...\n");
	kGetCh();
	kReboot();
}

static void kSetTimer(const char* pcParameterBuffer){
	PARAMETERLIST stList;
	char vcParameter[100];
	long lMillisecond;
	BOOL bPeriodic;

	// ÆÄ¶ó¹ÌÅÍ ÃÊ±âÈ­
	kInitializeParameter(&stList, pcParameterBuffer);

	// 1¹øÂ° ÆÄ¶ó¹ÌÅÍ Ãëµæ : Millisecond
	if(kGetNextParameter(&stList, vcParameter) == 0){
		kPrintf("Wrong Usage, ex) settimer 1(ms) 1(periodic)\n");
		return;
	}

	lMillisecond = kAToI(vcParameter, 10);

	// 2¹øÂ° ÆÄ¶ó¹ÌÅÍ Ãëµæ : Periodic
	if(kGetNextParameter(&stList, vcParameter) == 0){
		kPrintf("Wrong Usage, ex) settimer 1(ms) 1(periodic)\n");
		return;
	}

	bPeriodic = kAToI(vcParameter, 10);

	// PIT ÃÊ±âÈ­
	kInitializePIT(MSTOCOUNT(lMillisecond), bPeriodic);

	kPrintf("Time = %d ms, Periodic = %d Change Complete\n", lMillisecond, bPeriodic);
}

static void kWaitUsingPIT(const char* pcParameterBuffer){
	char vcParameter[100];
	int iLength;
	PARAMETERLIST stList;
	long lMillisecond;
	int i;

	// ÆÄ¶ó¹ÌÅÍ ÃÊ±âÈ­
	kInitializeParameter(&stList, pcParameterBuffer);

	// 1¹øÂ° ÆÄ¶ó¹ÌÅÍ Ãëµæ : Millisecond
	if(kGetNextParameter(&stList, vcParameter) == 0){
		kPrintf("Wrong Usage, ex) wait 1(ms)\n");
		return;
	}

	lMillisecond = kAToI(vcParameter, 10);

	kPrintf("%d ms Sleep Start...\n", lMillisecond);

	// ÀÎÅÍ·´Æ®¸¦ ºñÈ°¼ºÈ­ÇÏ°í, PIT ÄÁÆ®·Ñ·¯¸¦ ÅëÇØ Á÷Á¢ ½Ã°£À» ÃøÁ¤
	kDisableInterrupt();
	for(i = 0; i < (lMillisecond / 30); i++){
		kWaitUsingDirectPIT(MSTOCOUNT(30));
	}
	kWaitUsingDirectPIT(MSTOCOUNT(lMillisecond % 30));
	kEnableInterrupt();

	kPrintf("%d ms Sleep Complete\n", lMillisecond);

	// Å¸ÀÌ¸Ó º¹¿ø
	kInitializePIT(MSTOCOUNT(1), TRUE);
}

static void kReadTimeStampCounter(const char* pcParameterBuffer){
	QWORD qwTSC;

	qwTSC = kReadTSC();

	kPrintf("Time Stamp Counter = %q\n", qwTSC);
}

static void kMeasureProcessorSpeed(const char* pcParameterBuffer){
	int i;
	QWORD qwLastTSC, qwTotalTSC = 0;

	kPrintf("Now Measuring");

	// 10ÃÊ µ¿¾È º¯È­ÇÑ Time Stamp Counter¸¦ ÀÌ¿ëÇÏ¿© ÇÁ·Î¼¼¼­ÀÇ ¼Óµµ¸¦ °£Á¢ÀûÀ¸·Î ÃøÁ¤
	kDisableInterrupt();
	for(i = 0; i < 200; i++){
		qwLastTSC = kReadTSC();
		kWaitUsingDirectPIT(MSTOCOUNT(50));
		qwTotalTSC += (kReadTSC() - qwLastTSC);
		kPrintf(".");
	}
	// Å¸ÀÌ¸Ó º¹¿ø
	kInitializePIT(MSTOCOUNT(1), TRUE);
	kEnableInterrupt();

	kPrintf("\nCPU Speed = %d MHz\n", qwTotalTSC / 10 / 1000 / 1000);
}

static void kShowDateAndTime(const char* pcParameterBuffer){
	WORD wYear;
	BYTE bMonth, bDayOfMonth, bDayOfWeek;
	BYTE bHour, bMinute, bSecond;

	// RTC ÄÁÆ®·Ñ·¯¿¡¼­ ÇöÀç ½Ã°£ ¹× ³¯Â¥¸¦ ÀÐÀ½
	kReadRTCTime(&bHour, &bMinute, &bSecond);
	kReadRTCDate(&wYear, &bMonth, &bDayOfMonth, &bDayOfWeek);

	kPrintf("Date: %d-%d-%d %d:%d:%d (%s)\n", wYear, bMonth, bDayOfMonth, bHour, bMinute, bSecond, kConvertDayOfWeekToString(bDayOfWeek));
}

// Å×½ºÆ® ÅÂ½ºÅ© 1 : È­¸é Å×µÎ¸®¸¦ µ¹¸é¼­ ¹®ÀÚ¸¦ Ãâ·Â
static void kTestTask1(void){
	BYTE bData;
	int i = 0, iX = 0, iY = 0, iMargin, j;
	CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
	TCB* pstRunningTask;

	// TCB IDÀÇ ÀÏ·Ã ¹øÈ£¸¦ È­¸é ¿ÀÇÁ¼ÂÀ¸·Î ÀÌ¿ë
	pstRunningTask = kGetRunningTask();
	iMargin = (pstRunningTask->stLink.qwID & 0xFFFFFFFF) % 10;

	for(j = 0; j < 20000; j++){
		switch(i){
		case 0:
			iX++;
			if(iX >= (CONSOLE_WIDTH - iMargin)){
				i = 1;
			}
			break;

		case 1:
			iY++;
			if(iY >= (CONSOLE_HEIGHT - iMargin)){
				i = 2;
			}
			break;

		case 2:
			iX--;
			if(iX < iMargin){
				i = 3;
			}
			break;

		case 3:
			iY--;
			if(iY < iMargin){
				i = 0;
			}
			break;
		}

		pstScreen[iY * CONSOLE_WIDTH + iX].bCharacter = bData;
		pstScreen[iY * CONSOLE_WIDTH + iX].bAttribute = bData & 0x0F;
		bData++;

		// ¶ó¿îµå ·Îºó ½ºÄÉÁÙ·¯->¸ÖÆ¼·¹º§ Å¥ ½ºÄÉÁÙ·¯·Î ¾÷±×·¹ÀÌµåÇß±â ¶§¹®¿¡, ÅÂ½ºÅ© ÀüÈ¯Àº ÁÖ¼® Ã³¸®
		//kSchedule();
	}

	// ½ºÅÃ¿¡ º¹±Í ÁÖ¼Ò¸¦ »ðÀÔÇß±â ¶§¹®¿¡, ÅÂ½ºÅ© Á¾·á´Â ÁÖ¼® Ã³¸®
	//kExitTask();
}

// Å×½ºÆ® ÅÂ½ºÅ© 2 : ÀÚ½ÅÀÇ TCB IDÀÇ ÀÏ·Ã ¹øÈ£¸¦ Âü°íÇÏ¿© Æ¯Á¤ À§Ä¡¿¡ È¸ÀüÇÏ´Â ¹Ù¶÷°³ºñ¸¦ Ãâ·Â
static void kTestTask2(void){
	int i = 0, iOffset;
	CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
	TCB* pstRunningTask;
	char vcData[4] = {'-', '\\', '|', '/'};

	// ÇöÀç ÅÂ½ºÅ© IDÀÇ ¿ÀÇÁ¼ÂÀ» È­¸é ¿ÀÇÁ¼ÂÀ¸·Î »ç¿ë
	pstRunningTask = kGetRunningTask();
	iOffset = (pstRunningTask->stLink.qwID & 0xFFFFFFFF) * 2;
	iOffset = (CONSOLE_WIDTH * CONSOLE_HEIGHT) - (iOffset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));

	// ¹«ÇÑ ·çÇÁ
	while(1){

		// È¸ÀüÇÏ´Â ¹Ù¶÷°³ºñ Ãâ·Â
		pstScreen[iOffset].bCharacter = vcData[i % 4];
		pstScreen[iOffset].bAttribute = (iOffset % 15) + 1;
		i++;

		// ¶ó¿îµå ·Îºó ½ºÄÉÁÙ·¯->¸ÖÆ¼·¹º§ Å¥ ½ºÄÉÁÙ·¯·Î ¾÷±×·¹ÀÌµåÇß±â ¶§¹®¿¡, ÅÂ½ºÅ© ÀüÈ¯Àº ÁÖ¼® Ã³¸®
		//kSchedule();
	}
}

static void kCreateTestTask(const char* pcParameterBuffer){
	PARAMETERLIST stList;
	char vcParameter[100];
	long lType, lCount;
	int i;

	// ÆÄ¶ó¹ÌÅÍ ÃÊ±âÈ­
	kInitializeParameter(&stList, pcParameterBuffer);

	// 1¹øÂ° ÆÄ¶ó¹ÌÅÍ Ãëµæ : Type
	if(kGetNextParameter(&stList, vcParameter) == 0){
		kPrintf("Wrong Usage, ex) createtask 1(type) 1022(count)\n");
		return;
	}

	lType = kAToI(vcParameter, 10);

	// 2¹øÂ° ÆÄ¶ó¹ÌÅÍ Ãëµæ : Count
	if(kGetNextParameter(&stList, vcParameter) == 0){
		kPrintf("Wrong Usage, ex) createtask 1(type) 1022(count)\n");
		return;
	}

	lCount = kAToI(vcParameter, 10);

	switch(lType){
	case 1: // Å×½ºÆ® ÅÂ½ºÅ© 1 : Å×µÎ¸® ¹®ÀÚ Ãâ·Â
		for(i = 0; i < lCount; i++){
			if(kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kTestTask1) == NULL){
				break;
			}
		}

		kPrintf("kTestTask1 Created(%d)\n", i);
		break;

	case 2: // Å×½ºÆ® ÅÂ½ºÅ© 2 : ¹Ù¶÷°³ºñ Ãâ·Â
	default:
		for(i = 0; i < lCount; i++){
			if(kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kTestTask2) == NULL){
				break;
			}
		}

		kPrintf("kTestTask2 Created(%d)\n", i);
		break;
	}
}

static void kChangeTaskPriority(const char* pcParameterBuffer){
	PARAMETERLIST stList;
	char vcTaskID[30];
	char vcPriority[30];
	QWORD qwTaskID;
	BYTE bPriority;

	// ÆÄ¶ó¹ÌÅÍ ÃÊ±âÈ­
	kInitializeParameter(&stList, pcParameterBuffer);

	// 1¹øÂ° ÆÄ¶ó¹ÌÅÍ Ãëµæ : TaskID
	if(kGetNextParameter(&stList, vcTaskID) == 0){
		kPrintf("Wrong Usage, ex) changepriority 0x300000002(taskId) 0(priority)\n");
		return;
	}

	// 2¹øÂ° ÆÄ¶ó¹ÌÅÍ Ãëµæ : Priority
	if(kGetNextParameter(&stList, vcPriority) == 0){
		kPrintf("Wrong Usage, ex) changepriority 0x300000002(taskId) 0(priority)\n");
		return;
	}

	if(kMemCmp(vcTaskID, "0x", 2) == 0){
		qwTaskID = kAToI(vcTaskID + 2, 16);

	}else{
		qwTaskID = kAToI(vcTaskID, 10);
	}

	bPriority = kAToI(vcPriority, 10);

	kPrintf("Change Task Priority : TaskID=[0x%q], Priority=[%d] : ", qwTaskID, bPriority);

	if(kChangePriority(qwTaskID, bPriority) == TRUE){
		kPrintf("Success~!!\n");

	}else{
		kPrintf("Fail~!!\n");
	}
}

static void kShowTaskList(const char* pcParameterBuffer){
	int i;
	TCB* pstTCB;
	int iCount = 0;
	int iTaskCount;
	char vcLinePadding[4] = {0, };

	iTaskCount = kGetTaskCount();

	// Task Total Count ¶óÀÎ Ã¤¿ì±â
	if(iTaskCount < 10){
		kMemCpy(vcLinePadding, "===", 3);

	}else if(iTaskCount < 100){
		kMemCpy(vcLinePadding, "==", 2);

	}else if(iTaskCount < 1000){
		kMemCpy(vcLinePadding, "=", 1);
	}

	kPrintf("=========================== Task Total Count [%d] ===========================%s\n", iTaskCount, vcLinePadding);

	for(i = 0; i < TASK_MAXCOUNT; i++){
		pstTCB = kGetTCBInTCBPool(i);

		// ÅÂ½ºÅ© IDÀÇ »óÀ§ 32ºñÆ®(TCB ÇÒ´ç È½¼ö)°¡ 0ÀÌ ¾Æ´ÑÁö È®ÀÎ
		if((pstTCB->stLink.qwID >> 32) != 0){

			// ÅÂ½ºÅ© Á¤º¸¸¦ 5°³ Ãâ·Â½Ã¸¶´Ù Á¤º¸¸¦ ´õ Ãâ·ÂÇÒÁö ¿©ºÎ¸¦ È®ÀÎ
			if((iCount != 0) && ((iCount % 5) == 0)){

				kPrintf("Press any key to continue...('q' is exit):");

				if(kGetCh() == 'q'){
					kPrintf("\n");
					break;
				}

				kPrintf("\n");
			}

			// ÅÂ½ºÅ© ¸®½ºÆ® Ãâ·Â Á¤º¸
			// 1. TID  : Task ID
			// 2. PRI  : Priority
			// 3. FG   : Flags
			// 4. CTH  : Child Thread Count
			// 5. PPID : Parent Process ID
			// 6. MA   : Memory Address
			// 7. MS   : Memory Size
			// 8. S    : System Task
			// 9. P/T  : Process/Thread
			kPrintf("[%d] TID=[0x%Q], PRI=[%d], FG=[0x%Q], CTH=[%d]\n"
					, 1 + iCount++, pstTCB->stLink.qwID, GETPRIORITY(pstTCB->qwFlags), pstTCB->qwFlags, kGetListCount(&(pstTCB->stChildThreadList)));
			kPrintf("     PPID=[0x%Q], MA=[0x%Q], MS=[0x%Q], S=[%d], P/T=[%d/%d]\n"
					, pstTCB->qwParentProcessID, pstTCB->pvMemoryAddress, pstTCB->qwMemorySize
					, (pstTCB->qwFlags & TASK_FLAGS_SYSTEM) ? 1 : 0
					, (pstTCB->qwFlags & TASK_FLAGS_PROCESS) ? 1 : 0
					, (pstTCB->qwFlags & TASK_FLAGS_THREAD) ? 1 : 0);
			kPrintf("-------------------------------------------------------------------------------\n");
		}
	}
}

static void kKillTask(const char* pcParameterBuffer){
	PARAMETERLIST stList;
	char vcTaskID[30];
	QWORD qwTaskID;
	TCB* pstTCB;
	int i;

	// ÆÄ¶ó¹ÌÅÍ ÃÊ±âÈ­
	kInitializeParameter(&stList, pcParameterBuffer);

	// 1¹øÂ° ÆÄ¶ó¹ÌÅÍ Ãëµæ : TaskID
	if(kGetNextParameter(&stList, vcTaskID) == 0){
		kPrintf("Wrong Usage, ex) killtask 0x300000002(taskId) or all\n");
		return;
	}

	// Æ¯Á¤ IDÀÇ ÅÂ½ºÅ©¸¸ Á¾·á
	if(kMemCmp(vcTaskID, "all", 3) != 0){

		if(kMemCmp(vcTaskID, "0x", 2) == 0){
			qwTaskID = kAToI(vcTaskID + 2, 16);

		}else{
			qwTaskID = kAToI(vcTaskID, 10);
		}

		// [ÁÖÀÇ]ÆÄ¶ó¹ÌÅÍ TaskID¸¦ ÅÂ½ºÅ© Ç®ÀÇ ½ÇÁ¦ TaskID·Î µ¤¾î ½èÀ¸¹Ç·Î, ÆÄ¶ó¹ÌÅÍ TaskIDÀÇ ¿ÀÇÁ¼Â(ÇÏÀ§ 32ºñÆ®)Á¤º¸¸¸ Á¦´ë·Î ÀÔ·ÂÇÏ¸é ÇØ´ç ÅÂ½ºÅ©°¡ Á¾·áµÊ
		pstTCB = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
		qwTaskID = pstTCB->stLink.qwID;

		// ÇÒ´ç(»ý¼º)µÇÁö ¾ÊÀº ÅÂ½ºÅ©¿Í ½Ã½ºÅÛ ÅÂ½ºÅ©´Â Á¾·á ´ë»ó¿¡¼­ Á¦¿Ü
		if(((qwTaskID >> 32) != 0) && ((pstTCB->qwFlags & TASK_FLAGS_SYSTEM) == 0)){
			kPrintf("Kill Task : TaskID=[0x%q] : ", qwTaskID);

			if(kEndTask(qwTaskID) == TRUE){
				kPrintf("Success~!!\n");

			}else{
				kPrintf("Fail~!!\n");
			}

		}else{
			if((qwTaskID >> 32) == 0){
				kPrintf("Kill Task Fail : Task does not exist.\n");

			}else if((pstTCB->qwFlags & TASK_FLAGS_SYSTEM) != 0){
				kPrintf("Kill Task Fail : Task is system task.\n");

			}else{
				kPrintf("Kill Task Fail\n");
			}
		}

	// ¸ðµç ÅÂ½ºÅ© Á¾·á(´Ü, ÄÜ¼Ö ½© ÅÂ½ºÅ©, À¯ÈÞ ÅÂ½ºÅ©´Â Á¦¿Ü)
	}else{
		for(i = 0; i < TASK_MAXCOUNT; i++){
			pstTCB = kGetTCBInTCBPool(i);
			qwTaskID = pstTCB->stLink.qwID;

			// ÇÒ´ç(»ý¼º)µÇÁö ¾ÊÀº ÅÂ½ºÅ©¿Í ½Ã½ºÅÛ ÅÂ½ºÅ©´Â Á¾·á ´ë»ó¿¡¼­ Á¦¿Ü
			if(((qwTaskID >> 32) != 0) && ((pstTCB->qwFlags & TASK_FLAGS_SYSTEM) == 0)){
				kPrintf("Kill Task : TaskID=[0x%q] : ", qwTaskID);

				if(kEndTask(qwTaskID) == TRUE){
					kPrintf("Success~!!\n");

				}else{
					kPrintf("Fail~!!\n");
				}
			}
		}
	}
}

static void kCPULoad(const char* pcParameterBuffer){
	kPrintf("Processor Load: %d %%\n", kGetProcessorLoad());
}

/***** Àü¿ª º¯¼ö ¼±¾ð *****/
// ¹ÂÅØ½º Å×½ºÆ®¿ë ¹ÂÅØ½º¿Í º¯¼ö
static MUTEX gs_stMutex;
static volatile QWORD gs_qwAdder;

static void kPrintNumberTask(const char* pcParameterBuffer){
	int i, j;
	QWORD qwTickCount;

	// 50ms Á¤µµ ´ë±âÇÏ¿©, ¹ÂÅØ½º Å×½ºÆ®¿ë Ãâ·Â ¸Þ¼¼Áö¿Í ÄÜ¼Ö ½©ÀÇ Ãâ·Â ¸Þ¼¼Áö°¡ °ãÄ¡Áö ¾Êµµ·Ï ÇÔ
	qwTickCount = kGetTickCount();
	while((kGetTickCount() - qwTickCount) < 50){
		kSchedule();
	}

	// ¹ÂÅØ½º Å×½ºÆ®¿ë ¼ýÀÚ Ãâ·Â
	for(i = 0; i < 5; i++){
		kLock(&(gs_stMutex));

		kPrintf("Test Mutex : TaskID=[0x%Q] Value=[%d]\n", kGetRunningTask()->stLink.qwID, gs_qwAdder);
		gs_qwAdder++;

		kUnlock(&(gs_stMutex));

		// ÇÁ·Î¼¼¼­ÀÇ ¼Ò¸ð¸¦ ´Ã¸®·Á°í Ãß°¡ÇÑ ÄÚµå
		for(j = 0; j < 30000; j++);
	}

	// ¸ðµç ÅÂ½ºÅ©°¡ ¼ýÀÚ Ãâ·ÂÀ» ¿Ï·áÇÒ ¶§±îÁö 1000ms Á¤µµ ´ë±âÇÏ¿©, ¹ÂÅØ½º Å×½ºÆ®¿ë Ãâ·Â ¸Þ¼¼Áö¿Í À¯ÈÞ ÅÂ½ºÅ©ÀÇ ÅÂ½ºÅ© Á¾·á ¸Þ¼¼Áö°¡ °ãÄ¡Áö ¾Êµµ·Ï ÇÔ
	qwTickCount = kGetTickCount();
	while((kGetTickCount() - qwTickCount) < 1000){
		kSchedule();
	}

	// ½ºÅÃ¿¡ º¹±Í ÁÖ¼Ò¸¦ »ðÀÔÇß±â ¶§¹®¿¡, ÅÂ½ºÅ© Á¾·á´Â ÁÖ¼® Ã³¸®
	// kExitTask();
}

static void kTestMutex(const char* pcParameterBuffer){
	int i;

	gs_qwAdder = 1;

	// ¹ÂÅØ½º ÃÊ±âÈ­
	kInitializeMutex(&gs_stMutex);

	// ¹ÂÅØ½º Å×½ºÆ®¿ë ÅÂ½ºÅ© 3°³ »ý¼º
	for(i = 0; i < 3; i++){
		kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kPrintNumberTask);
	}

	kPrintf("Wait for the mutex test until [%d] tasks end...\n", i);
	kGetCh();
}

static void kCreateThreadTask(void){
	int i;

	for(i = 0; i < 3; i++){
		kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kTestTask2);
	}

	while(1){
		kSleep(1);
	}
}

static void kTestThread(const char* pcParameterBuffer){
	TCB* pstProcess;

	// ÇÁ·Î¼¼½º 1°³¿Í ½º·¹µå 3°³¸¦ »ý¼º
	pstProcess = kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_PROCESS, (void*)0xEEEEEEEE, 0x1000, (QWORD)kCreateThreadTask);

	if(pstProcess != NULL){
		kPrintf("Process [0x%Q] Create Success\n", pstProcess->stLink.qwID);

	}else{
		kPrintf("Process Create Fail\n");
	}
}

/***** Àü¿ª º¯¼ö Á¤ÀÇ *****/
static volatile QWORD gs_qwRandomValue = 0;

QWORD kRandom(void){
	gs_qwRandomValue = (gs_qwRandomValue * 412153 + 5571031) >> 16;
	return gs_qwRandomValue;
}

static void kDropCharacterThread(void){
	int iX;
	int i;
	char vcText[2] = {0, };

	iX = kRandom() % CONSOLE_WIDTH;

	while(1){
		kSleep(kRandom() % 20);

		if((kRandom() % 20) < 15){
			vcText[0] = ' ';
			for(i = 0; i < CONSOLE_HEIGHT - 1; i++){
				kPrintStringXY(iX, i, vcText);
				kSleep(50);
			}

		}else{
			for(i = 0; i < CONSOLE_HEIGHT - 1; i++){
				vcText[0] = i + kRandom();
				kPrintStringXY(iX, i, vcText);
				kSleep(50);
			}
		}
	}
}

static void kMatrixProcess(void){
	int i;

	for(i = 0; i < 300; i++){
		if(kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kDropCharacterThread) == NULL){
			break;
		}

		kSleep(kRandom() % 5 + 5);
	}

	kPrintf("%d Threads are created.\n", i);

	// Å°°¡ ÀÔ·ÂµÇ¸é ÇÁ·Î¼¼½º Á¾·á
	kGetCh();
}

static void kShowMatrix(const char* pcParameterBuffer){
	TCB* pstProcess;

	pstProcess = kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_PROCESS, (void*)0xE00000, 0xE00000, (QWORD)kMatrixProcess);

	if(pstProcess != NULL){
		kPrintf("Matrix Process [0x%Q] Create Success\n", pstProcess->stLink.qwID);

		// ÇÁ·Î¼¼½º°¡ Á¾·áµÉ ¶§±îÁö ´ë±â
		while((pstProcess->stLink.qwID >> 32) != 0){
			kSleep(100);
		}

	}else{
		kPrintf("Matrix Process Create Fail\n");
	}
}

static void kFPUTestTask(void){
	double dValue1;
	double dValue2;
	TCB* pstRunningTask;
	QWORD qwCount = 0;
	QWORD qwRandomValue;
	int i;
	int iOffset;
	char vcData[4] = {'-', '\\', '|', '/'};
	CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;

	// ÇöÀç ÅÂ½ºÅ© IDÀÇ ¿ÀÇÁ¼ÂÀ» È­¸é ¿ÀÇÁ¼ÂÀ¸·Î »ç¿ë
	pstRunningTask = kGetRunningTask();
	iOffset = (pstRunningTask->stLink.qwID & 0xFFFFFFFF) * 2;
	iOffset = (CONSOLE_WIDTH * CONSOLE_HEIGHT) - (iOffset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));

	// ¹«ÇÑ ·çÇÁ
	while(1){
		dValue1 = 1;
		dValue2 = 1;

		// Å×½ºÆ®¸¦ À§ÇØ µ¿ÀÏÇÑ °è»êÀ» 2¹ø ¹Ýº¹ÇØ¼­ ½ÇÇà
		for(i = 0; i < 10; i++){
			qwRandomValue = kRandom();
			dValue1 *= (double)qwRandomValue;
			dValue2 *= (double)qwRandomValue;

			kSleep(1);

			qwRandomValue = kRandom();
			dValue1 /= (double)qwRandomValue;
			dValue2 /= (double)qwRandomValue;

		}

		// FPU ¿¬»ê¿¡ ¹®Á¦°¡ ¹ß»ýÇÑ °æ¿ì, ¿¡·¯ ¸Þ¼¼Áö¸¦ Ãâ·ÂÇÏ°í ÅÂ½ºÅ© Á¾·á
		if(dValue1 != dValue2){
			kPrintf("Value is not same~!!, [%f] != [%f]\n", dValue1, dValue2);
			break;
		}

		// FPU ¿¬»ê¿¡ ¹®Á¦°¡ ¹ß»ýÇÏÁö ¾ÊÀº °æ¿ì, È¸ÀüÇÏ´Â ¹Ù¶÷°³ºñ¸¦ Ãâ·Â
		pstScreen[iOffset].bCharacter = vcData[qwCount % 4];
		pstScreen[iOffset].bAttribute = (iOffset % 15) + 1;
		qwCount++;
	}
}

static void kTestPIE(const char* pcParameterBuffer){
	double dResult;
	int i;

	// ÆÄÀÌ °è»ê ÈÄ, Ãâ·Â
	kPrintf("PIE Calculation Test\n");
	kPrintf("PIE : 355 / 113 = ");
	dResult = (double)355 / 113;
	//kPrintf("%d.%d%d\n", (QWORD)dResult, ((QWORD)(dResult * 10) % 10), ((QWORD)(dResult * 100) % 10));
	kPrintf("%f\n", dResult);

	// ½Ç¼ö¸¦ °è»êÇÏ´Â ÅÂ½ºÅ©(È¸ÀüÇÏ´Â ¹Ù¶÷°³ºñ)¸¦ 100°³ »ý¼º
	for(i = 0; i < 100; i++){
		kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kFPUTestTask);
	}
}

static void kShowDynamicMemoryInformation(const char* pcParameterBuffer){
	QWORD qwStartAddress, qwTotalSize, qwMetaSize, qwUsedSize;
	QWORD qwEndAddredss;
	QWORD qwTotalRAMSize;

	kGetDynamicMemoryInformation(&qwStartAddress, &qwTotalSize, &qwMetaSize, &qwUsedSize);
	qwEndAddredss = qwStartAddress + qwTotalSize;
	qwTotalRAMSize = kGetTotalRAMSize();

	kPrintf("========================= Dynamic Memory Information ==========================\n");
	kPrintf("Start Address  : 0x%Q byte, %d MB\n", qwStartAddress, qwStartAddress / 1024 / 1024);
	kPrintf("End Address    : 0x%Q byte, %d MB\n", qwEndAddredss, qwEndAddredss / 1024 / 1024);
	kPrintf("Total Size     : 0x%Q byte, %d MB\n", qwTotalSize, qwTotalSize / 1024 / 1024);
	kPrintf("Meta Size      : 0x%Q byte, %d KB\n", qwMetaSize, qwMetaSize / 1024);
	kPrintf("Used Size      : 0x%Q byte, %d KB\n", qwUsedSize, qwUsedSize / 1024);
	kPrintf("-------------------------------------------------------------------------------\n");
	kPrintf("Total RAM Size : 0x%Q byte, %d MB\n", qwTotalRAMSize * 1024 * 1024, qwTotalRAMSize);
	kPrintf("===============================================================================\n");
}

static void kTestSequentialAllocation(const char* pcParameterBuffer){
	DYNAMICMEMORY* pstMemory;
	long i, j, k;
	QWORD* pqwBuffer;

	kPrintf("====>>>> Dynamic Memory Sequential Test\n");

	pstMemory = kGetDynamicMemoryManager();

	for(i = 0; i < pstMemory->iMaxLevelCount; i++){

		kPrintf("Block List [%d] Test Start~!!\n", i);

		// ¸ðµç Å©±âÀÇ ºí·ÏÀ» ÇÒ´ç¹Þ¾Æ¼­ °ªÀ» Ã¤¿î ÈÄ °Ë»ç
		kPrintf("Allocation and Compare: ");

		for(j = 0; j < (pstMemory->iBlockCountOfSmallestBlock >> i); j++){

			pqwBuffer = (QWORD*)kAllocateMemory(DYNAMICMEMORY_MIN_SIZE << i);
			if(pqwBuffer == NULL){
				kPrintf("\nAllocation Fail~!!\n");
				return;
			}

			// ÇÒ´ç ¹ÞÀº ¸Þ¸ð¸®¿¡ °ªÀ» Ã¤¿ò
			for(k = 0; k < ((DYNAMICMEMORY_MIN_SIZE << i) / 8); k++){
				pqwBuffer[k] = k;
			}

			// °Ë»ç
			for(k = 0; k < ((DYNAMICMEMORY_MIN_SIZE << i) / 8); k++){
				if(pqwBuffer[k] != k){
					kPrintf("\nCompare Fail~!!\n");
					return;
				}
			}

			// ÁøÇà°úÁ¤À» .À¸·Î Ç¥½Ã
			kPrintf(".");
		}

		// ÇÒ´ç¹ÞÀº ºí·ÏÀ» ¸ðµÎ ÇØÁ¦
		kPrintf("\nFree: ");
		for(j = 0; j < (pstMemory->iBlockCountOfSmallestBlock >> i); j++){
			if(kFreeMemory((void*)(pstMemory->qwStartAddress + ((DYNAMICMEMORY_MIN_SIZE << i) * j))) == FALSE){
				kPrintf("\nFree Fail~!!\n");
				return;
			}

			// ÁøÇà°úÁ¤À» .À¸·Î Ç¥½Ã
			kPrintf(".");
		}

		kPrintf("\n");
	}

	kPrintf("Test Complete~!!\n");
}

static void kRandomAllocationTask(void){
	TCB* pstTask;
	QWORD qwMemorySize;
	char vcBuffer[200];
	BYTE* pbAllocationBuffer;
	int i, j;
	int iY;

	pstTask = kGetRunningTask();
	iY = (pstTask->stLink.qwID) % 15 + 9;

	for(j = 0; j < 10; j++){
		// 1KB~32MB Å©±âÀÇ ¸Þ¸ð¸®¸¦ ÇÒ´ç¹ÞÀ½
		do{
			qwMemorySize = ((kRandom() % (32 * 1024)) + 1) * 1024;
			pbAllocationBuffer = (BYTE*)kAllocateMemory(qwMemorySize);

			// ¸Þ¸ð¸®¸¦ ÇÒ´ç¹ÞÁö ¸øÇßÀ» °æ¿ì, ´Ù¸¥ ÅÂ½ºÅ©°¡ ¸Þ¸ð¸®¸¦ »ç¿ëÇÏ°í ÀÖÀ» ¼ö ÀÖÀ¸¹Ç·Î Àá½Ã ´ë±âÇÑ ÈÄ ´Ù½Ã ½Ãµµ
			if(pbAllocationBuffer == 0){
				kSleep(1);
			}

		}while(pbAllocationBuffer == 0);

		kSPrintf(vcBuffer, "|Address=[0x%Q], Size=[0x%Q] Allocation Success~!!", pbAllocationBuffer, qwMemorySize);
		kPrintStringXY(20, iY, vcBuffer);
		kSleep(200);

		// ¹öÆÛ¸¦ ¹ÝÀ¸·Î ³ª´²¼­ ·£´ýÇÑ µ¥ÀÌÅÍ¸¦ ¶È°°ÀÌ Ã¤¿ò
		kSPrintf(vcBuffer, "|Address=[0x%Q], Size=[0x%Q] Data Write...", pbAllocationBuffer, qwMemorySize);
		kPrintStringXY(20, iY, vcBuffer);

		for(i = 0; i < (qwMemorySize / 2); i++){
			pbAllocationBuffer[i] = kRandom() & 0xFF;
			pbAllocationBuffer[i+(qwMemorySize/2)] = pbAllocationBuffer[i];
		}

		kSleep(200);

		// Ã¤¿î µ¥ÀÌÅÍ°¡ Á¤»óÀûÀÎÁö È®ÀÎ
		kSPrintf(vcBuffer, "|Address=[0x%Q], Size=[0x%Q] Data Verify...", pbAllocationBuffer, qwMemorySize);
		kPrintStringXY(20, iY, vcBuffer);

		for(i = 0; i < (qwMemorySize / 2); i++){
			if(pbAllocationBuffer[i] != pbAllocationBuffer[i+(qwMemorySize/2)]){
				kPrintf("TaskID=[0x%Q] Verify Fail~!!\n", pstTask->stLink.qwID);
				kExitTask();
			}
		}

		kFreeMemory(pbAllocationBuffer);
		kSleep(200);
	}

	kExitTask();
}

static void kTestRandomAllocation(const char* pcParameterBuffer){
	int i;

	kPrintf("====>>>> Dynamic Memory Random Test\n");

	for(i = 0; i < 1000; i++){
		kCreateTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD, 0, 0, (QWORD)kRandomAllocationTask);
	}
}

static void kShowHDDInformation(const char* pcParameterBuffer){
	HDDINFORMATION stHDD;
	char vcBuffer[100];

	// ÇÏµå µð½ºÅ© Á¤º¸¸¦ ÀÐÀ½
	if(kGetHDDInformation(&stHDD) == FALSE){
		kPrintf("HDD Information Read Fail~!!");
		return;
	}

	kPrintf("======================= Primary Master HDD Information ========================\n");

	// ¸ðµ¨ ¹øÈ£ Ãâ·Â
	kMemCpy(vcBuffer, stHDD.vwModelNumber, sizeof(stHDD.vwModelNumber));
	vcBuffer[sizeof(stHDD.vwModelNumber) - 1] = '\0';
	kPrintf("Model Number   : %s\n", vcBuffer);

	// ½Ã¸®¾ó ¹øÈ£ Ãâ·Â
	kMemCpy(vcBuffer, stHDD.vwSerialNumber, sizeof(stHDD.vwSerialNumber));
	vcBuffer[sizeof(stHDD.vwSerialNumber) - 1] = '\0';
	kPrintf("Serial Number  : %s\n", vcBuffer);

	// ½Ç¸°´õ ¼ö, Çìµå ¼ö, ½Ç¸°´õ´ç ¼½ÅÍ ¼ö Ãâ·Â
	kPrintf("Cylinder Count : %d\n", stHDD.wNumberOfCylinder);
	kPrintf("Head Count     : %d\n", stHDD.wNumberOfHead);
	kPrintf("Sector Count   : %d\n", stHDD.wNumberOfSectorPerCylinder);

	// ÃÑ ¼½ÅÍ ¼ö Ãâ·Â
	kPrintf("Total Sectors  : %d sector, %d MB\n", stHDD.dwTotalSectors, stHDD.dwTotalSectors / 2 / 1024);

	kPrintf("===============================================================================\n");
}

static void kReadSector(const char* pcParameterBuffer){
	PARAMETERLIST stList;
	char vcLBA[50];
	char vcSectorCount[50];
	DWORD dwLBA;
	int iSectorCount;
	char* pcBuffer;
	int i, j;
	BYTE bData;
	BOOL bExit = FALSE;

	// ÆÄ¶ó¹ÌÅÍ ÃÊ±âÈ­
	kInitializeParameter(&stList, pcParameterBuffer);

	// 1¹øÂ° ÆÄ¶ó¹ÌÅÍ Ãëµæ : LBA
	if(kGetNextParameter(&stList, vcLBA) == 0){
		kPrintf("Wrong Usage, ex) readsector 0(LBA) 10(count)\n");
		return;
	}

	// 2¹øÂ° ÆÄ¶ó¹ÌÅÍ Ãëµæ : SectorCount
	if(kGetNextParameter(&stList, vcSectorCount) == 0){
		kPrintf("Wrong Usage, ex) readsector 0(LBA) 10(count)\n");
		return;
	}

	dwLBA = kAToI(vcLBA, 10);
	iSectorCount = kAToI(vcSectorCount, 10);

	// ¼½ÅÍ ¼ö¸¸Å­ÀÇ ¸Þ¸ð¸®¸¦ ÇÒ´ç
	pcBuffer = (char*)kAllocateMemory(iSectorCount * 512);

	// ¼½ÅÍ ÀÐ±â ¼öÇà
	if(kReadHDDSector(TRUE, TRUE, dwLBA, iSectorCount, pcBuffer) == iSectorCount){
		kPrintf("HDD Sector Read : LBA=[%d], Count=[%d] : Success~!!", dwLBA, iSectorCount);

		// ¸Þ¸ð¸® ¹öÆÛÀÇ ³»¿ë Ãâ·Â
		for(j = 0; j < iSectorCount; j++){
			for(i = 0; i < 512; i++){
				if(!((j == 0) && (i == 0)) && ((i % 256) == 0)){
					kPrintf("\nPress any key to continue...('q' is exit):");
					if(kGetCh() == 'q'){
						bExit = TRUE;
						break;
					}
				}

				if((i % 16) == 0){
					kPrintf("\n[LBA:%d, Offset:%d]\t| ", dwLBA + j, i);
				}

				// ¸ðµÎ µÎ ÀÚ¸®·Î Ç¥½ÃÇÏ±â À§ÇØ, 16º¸´Ù ÀÛÀº °æ¿ì 0À» Ãß°¡
				bData = pcBuffer[j*512+i] & 0xFF;
				if(bData < 16){
					kPrintf("0");
				}

				kPrintf("%X ", bData);
			}

			if(bExit == TRUE){
				break;
			}
		}

		kPrintf("\n");

	}else{
		kPrintf("HDD Sector Read : Fail~!!\n");
	}

	kFreeMemory(pcBuffer);
}

static void kWriteSector(const char* pcParameterBuffer){
	PARAMETERLIST stList;
	char vcLBA[50];
	char vcSectorCount[50];
	DWORD dwLBA;
	int iSectorCount;
	char* pcBuffer;
	int i, j;
	BYTE bData;
	BOOL bExit = FALSE;
	static DWORD s_dwWriteCount = 0;

	// ÆÄ¶ó¹ÌÅÍ ÃÊ±âÈ­
	kInitializeParameter(&stList, pcParameterBuffer);

	// 1¹øÂ° ÆÄ¶ó¹ÌÅÍ Ãëµæ : LBA
	if(kGetNextParameter(&stList, vcLBA) == 0){
		kPrintf("Wrong Usage, ex) writesector 0(LBA) 10(count)\n");
		return;
	}

	// 2¹øÂ° ÆÄ¶ó¹ÌÅÍ Ãëµæ : SectorCount
	if(kGetNextParameter(&stList, vcSectorCount) == 0){
		kPrintf("Wrong Usage, ex) writesector 0(LBA) 10(count)\n");
		return;
	}

	dwLBA = kAToI(vcLBA, 10);
	iSectorCount = kAToI(vcSectorCount, 10);

	s_dwWriteCount++;

	// ¼½ÅÍ ¼ö¸¸Å­ÀÇ ¸Þ¸ð¸®¸¦ ÇÒ´ç¹ÞÀº ÈÄ, µ¥ÀÌÅÍ¸¦ Ã¤¿ò(µ¥ÀÌÅÍ ÆÐÅÏÀº LBA ¾îµå·¹½º(4 byte)¿Í ¾²±â ¼öÇà È½¼ö(4 byte)·Î »ý¼º)
	pcBuffer = (char*)kAllocateMemory(iSectorCount * 512);
	for(j = 0; j < iSectorCount; j++){
		for(i = 0; i < 512; i += 8){
			*(DWORD*)&(pcBuffer[j*512+i]) = dwLBA + j;
			*(DWORD*)&(pcBuffer[j*512+i+4]) = s_dwWriteCount;
		}
	}

	// ¼½ÅÍ ¾²±â ¼öÇà
	if(kWriteHDDSector(TRUE, TRUE, dwLBA, iSectorCount, pcBuffer) != iSectorCount){
		kPrintf("HDD Sector Write : Fail~!!\n");
	}

	kPrintf("HDD Sector Write : LBA=[%d], Count=[%d] : Success~!!", dwLBA, iSectorCount);

	// ¸Þ¸ð¸® ¹öÆÛÀÇ ³»¿ë Ãâ·Â
	for(j = 0; j < iSectorCount; j++){
		for(i = 0; i < 512; i++){
			if(!((j == 0) && (i == 0)) && ((i % 256) == 0)){
				kPrintf("\nPress any key to continue...('q' is exit):");
				if(kGetCh() == 'q'){
					bExit = TRUE;
					break;
				}
			}

			if((i % 16) == 0){
				kPrintf("\n[LBA:%d, Offset:%d]\t| ", dwLBA + j, i);
			}

			// ¸ðµÎ µÎ ÀÚ¸®·Î Ç¥½ÃÇÏ±â À§ÇØ, 16º¸´Ù ÀÛÀº °æ¿ì 0À» Ãß°¡
			bData = pcBuffer[j*512+i] & 0xFF;
			if(bData < 16){
				kPrintf("0");
			}

			kPrintf("%X ", bData);
		}

		if(bExit == TRUE){
			break;
		}
	}

	kPrintf("\n");

	kFreeMemory(pcBuffer);
}

static void kMountHDD(const char* pcParameterBuffer){
	if(kMount() == FALSE){
		kPrintf("HDD Mount Fail~!!\n");
		return;
	}

	kPrintf("HDD Mount Success~!!\n");
}

static void kFormatHDD(const char* pcParameterBuffer){
	if(kFormat() == FALSE){
		kPrintf("HDD Format Fail~!!\n");
		return;
	}

	kPrintf("HDD Format Success~!!\n");
}

static void kShowFileSystemInformation(const char* pcParameterBuffer){
	FILESYSTEMMANAGER stManager;

	kGetFileSystemInformation(&stManager);

	kPrintf("=========================== File System Information ===========================\n");
	kPrintf("Mounted                          : %s\n",         (stManager.bMounted == TRUE) ? "true" : "false");
	kPrintf("MBR Sector Count                 : %d sector\n",  (stManager.bMounted == TRUE) ? 1 : 0);
	kPrintf("Reserved Sector Count            : %d sector\n",  stManager.dwReservedSectorCount);
	kPrintf("Cluster Link Area Start Address  : %d sector\n",  stManager.dwClusterLinkAreaStartAddress);
	kPrintf("Cluster Link Area Size           : %d sector\n",  stManager.dwClusterLinkAreaSize);
	kPrintf("Data Area Start Address          : %d sector\n",  stManager.dwDataAreaStartAddress);
	kPrintf("Total Cluster Count              : %d cluster\n", stManager.dwTotalClusterCount);
	kPrintf("Cache Enable                     : %s\n",         (stManager.bCacheEnable == TRUE) ? "true" : "false");
	kPrintf("===============================================================================\n");
}

static void kCreateFileInRootDirectory(const char* pcParameterBuffer){
	PARAMETERLIST stList;
	char vcFileName[50];
	int iLength;
	FILE* pstFile;

	// ÆÄ¶ó¹ÌÅÍ ÃÊ±âÈ­
	kInitializeParameter(&stList, pcParameterBuffer);

	// 1¹øÂ° ÆÄ¶ó¹ÌÅÍ Ãëµæ : FileName
	if((iLength = kGetNextParameter(&stList, vcFileName)) == 0){
		kPrintf("Wrong Usage, ex) createfile a.txt\n");
		return;
	}

	vcFileName[iLength] = '\0';

	if(iLength > (FILESYSTEM_MAXFILENAMELENGTH - 1)){
		kPrintf("Wrong Usage, Too Long File Name\n");
		return;
	}

	// ÆÄÀÏ ¿­±â
	pstFile = fopen(vcFileName, "w");
	if(pstFile == NULL){
		kPrintf("'%s' File Create Fail~!!\n", vcFileName);
		return;
	}

	// ÆÄÀÏ ´Ý±â
	fclose(pstFile);
}

static void kDeleteFileInRootDirectory(const char* pcParameterBuffer){
	PARAMETERLIST stList;
	char vcFileName[50];
	int iLength;

	// ÆÄ¶ó¹ÌÅÍ ÃÊ±âÈ­
	kInitializeParameter(&stList, pcParameterBuffer);

	// 1¹øÂ° ÆÄ¶ó¹ÌÅÍ Ãëµæ : FileName
	if((iLength = kGetNextParameter(&stList, vcFileName)) == 0){
		kPrintf("Wrong Usage, ex) deletefile a.txt\n");
		return;
	}

	vcFileName[iLength] = '\0';

	if(iLength > (FILESYSTEM_MAXFILENAMELENGTH - 1)){
		kPrintf("Wrong Usage, Too Long File Name\n");
		return;
	}

	// ÆÄÀÏ »èÁ¦
	if(remove(vcFileName) != 0){
		kPrintf("'%s' File Delete Fail~!!\n", vcFileName);
		return;
	}
}

static void kShowRootDirectory(const char* pcParameterBuffer){
	DIR* pstDirectory;
	int i, iCount, iTotalCount;
	struct dirent* pstEntry;
	char vcBuffer[76]; // ¹öÆÛ Å©±â(76 byte)¸¦ ÅØ½ºÆ® ºñµð¿À ¸Þ¸ð¸®(È­¸é)ÀÇ ÇÑ ¶óÀÎ ±æÀÌ¿¡ ¸ÂÃã
	char vcTempValue[50];
	DWORD dwTotalByte;
	DWORD dwUsedClusterCount;
	FILESYSTEMMANAGER stManager;

	// ÆÄÀÏ ½Ã½ºÅÛ Á¤º¸ Ãëµæ
	kGetFileSystemInformation(&stManager);

	// ·çÆ® µð·ºÅä¸® ¿­±â(·çÆ® µð·ºÅä¸®¹Û¿¡ ¾øÀ¸¹Ç·Î, µð·ºÅä¸® ÀÌ¸§["/"]Àº ¹«½Ã)
	pstDirectory = opendir("/");
	if(pstDirectory == NULL){
		kPrintf("Root Directory Open Fail~!!\n");
		return;
	}

	// ·çÆ® µð·ºÅä¸®³»ÀÇ ÃÑ ÆÄÀÏ °³¼ö, ÃÑ ÆÄÀÏ Å©±â, »ç¿ëµÈ Å¬·¯½ºÅÍ °³¼ö¸¦ ±¸ÇÔ
	iTotalCount = 0;
	dwTotalByte = 0;
	dwUsedClusterCount = 0;
	while(1){
		// ·çÆ® µð·ºÅä¸® ÀÐ±â
		pstEntry = readdir(pstDirectory);
		if(pstEntry == NULL){
			break;
		}

		// ÃÑ ÆÄÀÏ °³¼ö, ÃÑ ÆÄÀÏ Å©±â °è»ê
		iTotalCount++;
		dwTotalByte += pstEntry->dwFileSize;

		// »ç¿ëµÈ Å¬·¯½ºÅÍ °³¼ö °è»ê
		if(pstEntry->dwFileSize == 0){
			// ÆÄÀÏ Å©±â°¡ 0ÀÎ °æ¿ì¶óµµ Å¬·¯½ºÅÍ 1°³´Â ÇÒ´çµÇ¾î ÀÖÀ½
			dwUsedClusterCount++;

		}else{
			// ÆÄÀÏ Å©±â¸¦ Å¬·¯½ºÅÍÀÇ Å©±â ´ÜÀ§·Î Á¤·Ä(¿Ã¸² Ã³¸®)ÇÏ¿©, »ç¿ëµÈ Å¬·¯½ºÅÍ °³¼ö¸¦ ±¸ÇÔ
			dwUsedClusterCount += ((pstEntry->dwFileSize + (FILESYSTEM_CLUSTERSIZE - 1)) / FILESYSTEM_CLUSTERSIZE);
		}
	}

	// ÆÄÀÏ ¸ñ·Ï Ãâ·Â ·çÇÁ Ã³¸®
	rewinddir(pstDirectory);
	iCount = 0;
	while(1){
		// ·çÆ® µð·ºÅä¸® ÀÐ±â
		pstEntry = readdir(pstDirectory);
		if(pstEntry == NULL){
			break;
		}

		// Ãâ·Â ¹öÆÛ¸¦ °ø¹éÀ¸·Î ÃÊ±âÈ­
		kMemSet(vcBuffer, ' ', sizeof(vcBuffer) - 1);
		vcBuffer[sizeof(vcBuffer)-1] = '\0';

		// Ãâ·Â ¹öÆÛ¿¡ ÆÄÀÏ ÀÌ¸§ ¼³Á¤
		kMemCpy(vcBuffer, pstEntry->d_name, kStrLen(pstEntry->d_name));

		// Ãâ·Â ¹öÆÛ¿¡ ÆÄÀÏ Å©±â ¼³Á¤
		kSPrintf(vcTempValue, "%d byte", pstEntry->dwFileSize);
		kMemCpy(vcBuffer + 30, vcTempValue, kStrLen(vcTempValue));

		// Ãâ·Â ¹öÆÛ¿¡ ½ÃÀÛ Å¬·¯½ºÅÍ ÀÎµ¦½º ¼³Á¤
		kSPrintf(vcTempValue, "0x%X cluster", pstEntry->dwStartClusterIndex);
		kMemCpy(vcBuffer + 55, vcTempValue, kStrLen(vcTempValue));

		// ÆÄÀÏ ¸ñ·Ï Ãâ·Â
		kPrintf("    %s\n", vcBuffer);

		// ÆÄÀÏ ¸ñ·ÏÀ» 15°³ Ãâ·Â½Ã¸¶´Ù ¸ñ·ÏÀ» ´õ Ãâ·ÂÇÒÁö ¿©ºÎ¸¦ È®ÀÎ
		if((iCount != 0) && ((iCount % 15) == 0)){

			kPrintf("Press any key to continue...('q' is exit):");

			if(kGetCh() == 'q'){
				kPrintf("\n");
				break;
			}

			kPrintf("\n");
		}

		iCount++;
	}

	// ÃÑ ÆÄÀÏ °³¼ö, ÃÑ ÆÄÀÏ Å©±â, ÇÏµå µð½ºÅ©ÀÇ ¿©À¯ °ø°£(³²Àº °ø°£)¸¦ Ãâ·Â
	kPrintf("\t\tTotal File Count : %d\n", iTotalCount);
	kPrintf("\t\tTotal File Size  : %d byte (%d cluster)\n", dwTotalByte, dwUsedClusterCount);
	kPrintf("\t\tFree Space       : %d KB (%d cluster)\n", (stManager.dwTotalClusterCount - dwUsedClusterCount) * FILESYSTEM_CLUSTERSIZE / 1024, stManager.dwTotalClusterCount - dwUsedClusterCount);

	// ·çÆ® µð·ºÅä¸® ´Ý±â
	closedir(pstDirectory);
}

static void kWriteDataToFile(const char* pcParameterBuffer){
	PARAMETERLIST stList;
	char vcFileName[50];
	int iLength;
	FILE* fp;
	int iEnterCount;
	BYTE bKey;

	// ÆÄ¶ó¹ÌÅÍ ÃÊ±âÈ­
	kInitializeParameter(&stList, pcParameterBuffer);

	// 1¹øÂ° ÆÄ¶ó¹ÌÅÍ Ãëµæ : FileName
	if((iLength = kGetNextParameter(&stList, vcFileName)) == 0){
		kPrintf("Wrong Usage, ex) writefile a.txt\n");
		return;
	}

	vcFileName[iLength] = '\0';

	if(iLength > (FILESYSTEM_MAXFILENAMELENGTH - 1)){
		kPrintf("Wrong Usage, Too Long File Name\n");
		return;
	}

	// ÆÄÀÏ ¿­±â
	fp = fopen(vcFileName, "w");
	if(fp == NULL){
		kPrintf("'%s' File Open Fail~!!\n", vcFileName);
		return;
	}

	// ÆÄÀÏ ¾²±â ·çÇÁ
	iEnterCount = 0;
	while(1){
		bKey = kGetCh();

		// ¿£ÅÍ Å°¸¦ 3¹ø ¿¬¼Ó ´­·¶À» °æ¿ì, Á¾·á
		if(bKey == KEY_ENTER){
			iEnterCount++;
			if(iEnterCount >= 3){
				break;
			}
		}else{
			iEnterCount = 0;
		}

		kPrintf("%c", bKey);

		// ÆÄÀÏ ¾²±â
		if(fwrite(&bKey, 1, 1, fp) != 1){
			kPrintf("'%s' File Write Fail~!!\n", vcFileName);
			break;
		}
	}

	// ÆÄÀÏ ´Ý±â
	fclose(fp);
}

static void kReadDataFromFile(const char* pcParameterBuffer){
	PARAMETERLIST stList;
	char vcFileName[50];
	int iLength;
	FILE* fp;
	int iEnterCount;
	BYTE bKey;

	// ÆÄ¶ó¹ÌÅÍ ÃÊ±âÈ­
	kInitializeParameter(&stList, pcParameterBuffer);

	// 1¹øÂ° ÆÄ¶ó¹ÌÅÍ Ãëµæ : FileName
	if((iLength = kGetNextParameter(&stList, vcFileName)) == 0){
		kPrintf("Wrong Usage, ex) readfile a.txt\n");
		return;
	}

	vcFileName[iLength] = '\0';

	if(iLength > (FILESYSTEM_MAXFILENAMELENGTH - 1)){
		kPrintf("Wrong Usage, Too Long File Name\n");
		return;
	}

	// ÆÄÀÏ ¿­±â
	fp = fopen(vcFileName, "r");
	if(fp == NULL){
		kPrintf("'%s' File Open Fail~!!\n", vcFileName);
		return;
	}

	// ÆÄÀÏ ÀÐ±â ·çÇÁ
	iEnterCount = 0;
	while(1){
		// ÆÄÀÏ ÀÐ±â
		if(fread(&bKey, 1, 1, fp) != 1){
			break;
		}

		kPrintf("%c", bKey);

		// ¿£ÅÍ Å°ÀÎ °æ¿ì, È½¼ö Áõ°¡
		if(bKey == KEY_ENTER){
			iEnterCount++;

			// ÆÄÀÏ ³»¿ëÀ» 15¶óÀÎ Ãâ·Â½Ã¸¶´Ù ³»¿ëÀ» ´õ Ãâ·ÂÇÒÁö ¿©ºÎ¸¦ È®ÀÎ
			if((iEnterCount != 0) && ((iEnterCount % 15) == 0)){

				kPrintf("Press any key to continue...('q' is exit):");

				if(kGetCh() == 'q'){
					kPrintf("\n");
					break;
				}

				kPrintf("\n");
				iEnterCount = 0;
			}
		}
	}

	// ÆÄÀÏ ´Ý±â
	fclose(fp);
}

static void kTestFileIO(const char* pcParameterBuffer){
	FILE* pstFile;
	BYTE* pbBuffer;
	int i;
	int j;
	DWORD dwRandomOffset;
	DWORD dwByteCount;
	BYTE vbTempBuffer[1024];
	DWORD dwMaxFileSize;

	kPrintf("====>>>> File I/O Test Start\n");

	// 4MB ¸Þ¸ð¸®(¹öÆÛ) ÇÒ´ç
	dwMaxFileSize = 4 * 1024 * 1024;
	pbBuffer = (BYTE*)kAllocateMemory(dwMaxFileSize);
	if(pbBuffer == NULL){
		kPrintf("Memory Allocation Fail~!!\n");
		return;
	}

	// Å×½ºÆ®¿ë ÆÄÀÏ »èÁ¦
	remove("testfileio.bin");

	//----------------------------------------------------------------------------------------------------
	// 1. ÆÄÀÏ ¿­±â ½ÇÆÐ Å×½ºÆ®
	//----------------------------------------------------------------------------------------------------
	kPrintf("1. File Open Fail Test...");

	// ÀÐ±â ¸ðµå(r)´Â ÆÄÀÏÀÌ Á¸ÀçÇÏÁö ¾Ê´Â °æ¿ì ÆÄÀÏÀ» »ý¼ºÇÏÁö ¾ÊÀ¸¹Ç·Î, NULLÀ» ¸®ÅÏ
	pstFile = fopen("testfileio.bin", "r");
	if(pstFile == NULL){
		kPrintf("[Pass]\n");

	}else{
		kPrintf("[Fail]\n");
		fclose(pstFile);
	}

	//----------------------------------------------------------------------------------------------------
	// 2. ÆÄÀÏ »ý¼º Å×½ºÆ®
	//----------------------------------------------------------------------------------------------------
	kPrintf("2. File Create Test...");

	// ¾²±â ¸ðµå(w)´Â ÆÄÀÏÀÌ Á¸ÀçÇÏÁö ¾Ê´Â °æ¿ì ÆÄÀÏÀ» »ý¼ºÇÏ¹Ç·Î, ÆÄÀÏ ÇÚµéÀ» ¸®ÅÏ
	pstFile = fopen("testfileio.bin", "w");
	if(pstFile != NULL){
		kPrintf("[Pass]\n");
		kPrintf("    FileHandle=[0x%Q]\n", pstFile);

	}else{
		kPrintf("[Fail]\n");
	}

	//----------------------------------------------------------------------------------------------------
	// 3. ¼øÂ÷ÀûÀÎ ¿µ¿ª ¾²±â Å×½ºÆ®
	//----------------------------------------------------------------------------------------------------
	kPrintf("3. Sequential Write Test(Cluster Size)...");

	// ¿­¸° ÆÄÀÏ¿¡ ¾²±â ¼öÇà
	for(i = 0; i < 100; i++){

		kMemSet(pbBuffer, i, FILESYSTEM_CLUSTERSIZE);

		// ÆÄÀÏ ¾²±â
		if(fwrite(pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile) != FILESYSTEM_CLUSTERSIZE){
			kPrintf("[Fail]\n");
			kPrintf("    %d Cluster Error\n", i);
			break;
		}
	}

	if(i >= 100){
		kPrintf("[Pass]\n");
	}

	//----------------------------------------------------------------------------------------------------
	// 4. ¼øÂ÷ÀûÀÎ ¿µ¿ª ÀÐ±â ¹× °ËÁõ Å×½ºÆ®
	//----------------------------------------------------------------------------------------------------
	kPrintf("4. Sequential Read and Verify Test(Cluster Size)...");

	// ÆÄÀÏÀÇ Ã³À½À¸·Î ÀÌµ¿
	fseek(pstFile, -100 * FILESYSTEM_CLUSTERSIZE, SEEK_END);

	// ¿­¸° ÆÄÀÏ¿¡¼­ ÀÐ±â ¼öÇà ÈÄ µ¥ÀÌÅÍ °ËÁõ
	for(i = 0; i < 100; i++){

		// ÆÄÀÏ ÀÐ±â
		if(fread(pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile) != FILESYSTEM_CLUSTERSIZE){
			kPrintf("[Fail]\n");
			break;
		}

		// µ¥ÀÌÅÍ °ËÁõ
		for(j = 0; j < FILESYSTEM_CLUSTERSIZE; j++){
			if(pbBuffer[j] != (BYTE)i){
				kPrintf("[Fail]\n");
				kPrintf("    %d Cluster Error, [0x%X] != [0x%X]", i, pbBuffer[j], (BYTE)i);
				break;
			}
		}
	}

	if(i >= 100){
		kPrintf("[Pass]\n");
	}

	//----------------------------------------------------------------------------------------------------
	// 5. ÀÓÀÇÀÇ ¿µ¿ª ¾²±â Å×½ºÆ®
	//----------------------------------------------------------------------------------------------------
	kPrintf("5. Random Write Test\n");

	// ¹öÆÛ¸¦ 0À¸·Î Ã¤¿ò
	kMemSet(pbBuffer, 0, dwMaxFileSize);

	// ÆÄÀÏÀÇ Ã³À½À¸·Î ÀÌµ¿ ÈÄ, ÆÄÀÏÀÇ ³»¿ëÀ» ÀÐ¾î¼­ ¹öÆÛ·Î º¹»ç
	fseek(pstFile, -100 * FILESYSTEM_CLUSTERSIZE, SEEK_CUR);
	fread(pbBuffer, 1, dwMaxFileSize, pstFile);

	// ÆÄÀÏ°ú ¹öÆÛÀÇ ÀÓÀÇÀÇ À§Ä¡¿¡ µ¿ÀÏÇÑ µ¥ÀÌÅÍ¸¦ ¾¸
	for(i = 0; i < 100; i++){
		dwByteCount = (kRandom() % (sizeof(vbTempBuffer) - 1)) + 1;
		dwRandomOffset = kRandom() % (dwMaxFileSize - dwByteCount);
		kPrintf("    [%d] Offset=[%d], Byte=[%d]...", i, dwRandomOffset, dwByteCount);

		// ÆÄÀÏÀÇ ÀÓÀÇÀÇ À§Ä¡¿¡ ¾²±â
		fseek(pstFile, dwRandomOffset, SEEK_SET);
		kMemSet(vbTempBuffer, i, dwByteCount);
		if(fwrite(vbTempBuffer, 1, dwByteCount, pstFile) != dwByteCount){
			kPrintf("[Fail]\n");
			break;

		}else{
			kPrintf("[Pass]\n");
		}

		// ¹öÆÛÀÇ ÀÓÀÇÀÇ À§Ä¡¿¡ ¾²±â
		kMemSet(pbBuffer + dwRandomOffset, i, dwByteCount);
	}

	// ÆÄÀÏ°ú ¹öÆÛÀÇ ¸¶Áö¸·À¸·Î ÀÌµ¿ÇÏ¿© 1¹ÙÀÌÆ®¸¦ ½á¼­, Å©±â¸¦ 4MB·Î ¸¸µê
	fseek(pstFile, dwMaxFileSize - 1, SEEK_SET);
	fwrite(&i, 1, 1, pstFile);
	pbBuffer[dwMaxFileSize - 1] = (BYTE)i;

	//----------------------------------------------------------------------------------------------------
	// 6. ÀÓÀÇÀÇ ¿µ¿ª ÀÐ±â ¹× °ËÁõ Å×½ºÆ®
	//----------------------------------------------------------------------------------------------------
	kPrintf("6. Random Read and Verify Test\n");

	// ÆÄÀÏ°ú ¹öÆÛÀÇ ÀÓÀÇÀÇ À§Ä¡¿¡¼­ µ¥ÀÌÅÍ¸¦ ÀÐÀº ÈÄ, µ¥ÀÌÅÍ °ËÁõ
	for(i = 0; i < 100; i++){
		dwByteCount = (kRandom() % (sizeof(vbTempBuffer) - 1)) + 1;
		dwRandomOffset = kRandom() % (dwMaxFileSize - dwByteCount);
		kPrintf("    [%d] Offset=[%d], Byte=[%d]...", i, dwRandomOffset, dwByteCount);

		// ÆÄÀÏÀÇ ÀÓÀÇÀÇ À§Ä¡¿¡¼­ ÀÐ±â
		fseek(pstFile, dwRandomOffset, SEEK_SET);
		if(fread(vbTempBuffer, 1, dwByteCount, pstFile) != dwByteCount){
			kPrintf("[Fail]\n");
			kPrintf("    File Read Fail\n");
			break;
		}

		// µ¥ÀÌÅÍ °ËÁõ
		if(kMemCmp(pbBuffer + dwRandomOffset, vbTempBuffer, dwByteCount) != 0){
			kPrintf("[Fail]\n");
			kPrintf("    Data Verify Fail\n");
			break;
		}

		kPrintf("[Pass]\n");
	}

	//----------------------------------------------------------------------------------------------------
	// 7. ¼øÂ÷ÀûÀÎ ¿µ¿ª ¾²±â, ÀÐ±â ¹× °ËÁõ Å×½ºÆ®
	//----------------------------------------------------------------------------------------------------
	kPrintf("7. Sequential Write, Read and Verify Test(1024 byte)\n");

	// ÆÄÀÏÀÇ Ã³À½À¸·Î ÀÌµ¿
	fseek(pstFile, -dwMaxFileSize, SEEK_CUR);

	// ÆÄÀÏ ¾²±â ·çÇÁ(¾ÕºÎºÐ¿¡ 2MB¸¸ ¾¸)
	for(i = 0; i < (2 * 1024 * 1024 / 1024); i++){
		kPrintf("    [%d] Offset=[%d], Byte=[%d] Write...", i, i * 1024, 1024);

		// ÆÄÀÏ ¾²±â(1024 byte¾¿ ¾¸)
		if(fwrite(pbBuffer + (i * 1024), 1, 1024, pstFile) != 1024){
			kPrintf("[Fail]\n");
			break;

		}else{
			kPrintf("[Pass]\n");
		}
	}

	// ÆÄÀÏÀÇ Ã³À½À¸·Î ÀÌµ¿
	fseek(pstFile, -dwMaxFileSize, SEEK_SET);

	// ÆÄÀÏ ÀÐ±â ÈÄ, µ¥ÀÌÅÍ °ËÁõ(Random Write·Î µ¥ÀÌÅÍ°¡ Àß¸ø ÀúÀåµÇ¾úÀ» ¼öµµ ÀÖÀ¸¹Ç·Î, °ËÁõÀº 4MB ÀüÃ¼ ¿µ¿ªÀ» ´ë»óÀ¸·Î ÇÔ)
	for(i = 0; i < (dwMaxFileSize / 1024); i++){
		kPrintf("    [%d] Offset=[%d], Byte=[%d] Read and Verify...", i, i * 1024, 1024);

		// ÆÄÀÏ ÀÐ±â(1024 byte¾¿ ÀÐÀ½)
		if(fread(vbTempBuffer, 1, 1024, pstFile) != 1024){
			kPrintf("[Fail]\n");
			break;
		}

		// µ¥ÀÌÅÍ °ËÁõ
		if(kMemCmp(pbBuffer + (i * 1024), vbTempBuffer, 1024) != 0){
			kPrintf("[Fail]\n");
			break;

		}else{
			kPrintf("[Pass]\n");
		}
	}

	//----------------------------------------------------------------------------------------------------
	// 8. ÆÄÀÏ »èÁ¦ ½ÇÆÐ Å×½ºÆ®
	//----------------------------------------------------------------------------------------------------
	kPrintf("8. File Delete Fail Test...");

	// ÇöÀç ÆÄÀÏÀÌ ¿­·Á ÀÖ´Â »óÅÂÀÌ¹Ç·Î, ÆÄÀÏ »èÁ¦¿¡ ½ÇÆÐÇØ¾ß ÇÔ
	if(remove("testfileio.bin") != 0){
		kPrintf("[Pass]\n");

	}else{
		kPrintf("[Fail]\n");
	}

	//----------------------------------------------------------------------------------------------------
	// 9. ÆÄÀÏ ´Ý±â Å×½ºÆ®
	//----------------------------------------------------------------------------------------------------
	kPrintf("9. File Close Fail Test...");

	// ÆÄÀÏ ´Ý±â
	if(fclose(pstFile) == 0){
		kPrintf("[Pass]\n");

	}else{
		kPrintf("[Fail]\n");
	}

	//----------------------------------------------------------------------------------------------------
	// 10. ÆÄÀÏ »èÁ¦ Å×½ºÆ®
	//----------------------------------------------------------------------------------------------------
	kPrintf("10. File Delete Test...");

	// ÆÄÀÏ »èÁ¦
	if(remove("testfileio.bin") == 0){
		kPrintf("[Pass]\n");

	}else{
		kPrintf("[Fail]\n");
	}

	// ¸Þ¸ð¸®(¹öÆÛ) ÇØÁ¦
	kFreeMemory(pbBuffer);

	kPrintf("====>>>> File I/O Test End\n");
}

static void kTestPerformance(const char* pcParameterBuffer){
	FILE* pstFile;
	DWORD dwClusterTestFileSize;
	DWORD dwOneByteTestFileSize;
	QWORD qwLastTickCount;
	DWORD i;
	BYTE* pbBuffer;

	// ÆÄÀÏ Å©±â ¼³Á¤(Å¬·¯½ºÅÍ ´ÜÀ§:1MB, ¹ÙÀÌÆ® ´ÜÀ§:16KB)
	dwClusterTestFileSize = 1024 * 1024;
	dwOneByteTestFileSize = 16 * 1024;

	// ¸Þ¸ð¸® ÇÒ´ç
	pbBuffer = (BYTE*)kAllocateMemory(dwClusterTestFileSize);
	if(pbBuffer == NULL){
		kPrintf("Memory Allocate Fail~!!\n");
		return;
	}

	kMemSet(pbBuffer, 0, FILESYSTEM_CLUSTERSIZE);

	kPrintf("====>>>> File I/O Performance Test Start\n");

	//----------------------------------------------------------------------------------------------------
	// 1-1. Å¬·¯½ºÅÍ ´ÜÀ§·Î ÆÄÀÏÀ» ¼øÂ÷ÀûÀ¸·Î ¾²´Â Å×½ºÆ®
	//----------------------------------------------------------------------------------------------------
	kPrintf("1. Sequential Read/Write Test (Cluster -> 1MB)\n");

	// ±âÁ¸ ÆÄÀÏÀ» »èÁ¦ÇÏ°í »õ·Î »ý¼º
	remove("performance.txt");
	pstFile = fopen("performance.txt", "w");
	if(pstFile == NULL){
		kPrintf("File Open Fail~!!\n");
		kFreeMemory(pbBuffer);
		return;
	}

	qwLastTickCount = kGetTickCount();

	// Å¬·¯½ºÅÍ ´ÜÀ§·Î ¾²´Â Å×½ºÆ®
	for(i = 0; i < (dwClusterTestFileSize / FILESYSTEM_CLUSTERSIZE); i++){
		if(fwrite(pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile) != FILESYSTEM_CLUSTERSIZE){
			kPrintf("File Write Fail~!!\n");
			fclose(pstFile);
			remove("performance.txt");
			kFreeMemory(pbBuffer);
			return;
		}
	}

	// Å×½ºÆ® ½Ã°£ Ãâ·Â
	kPrintf("    - Write : %d ms\n", kGetTickCount() - qwLastTickCount);

	//----------------------------------------------------------------------------------------------------
	// 1-2. Å¬·¯½ºÅÍ ´ÜÀ§·Î ÆÄÀÏÀ» ¼øÂ÷ÀûÀ¸·Î ÀÐ´Â Å×½ºÆ®
	//----------------------------------------------------------------------------------------------------

	// ÆÄÀÏÀÇ Ã³À½À¸·Î ÀÌµ¿
	fseek(pstFile, 0, SEEK_SET);

	qwLastTickCount = kGetTickCount();

	// Å¬·¯½ºÅÍ ´ÜÀ§·Î ÀÐ´Â Å×½ºÆ®
	for(i = 0; i < (dwClusterTestFileSize / FILESYSTEM_CLUSTERSIZE); i++){
		if(fread(pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile) != FILESYSTEM_CLUSTERSIZE){
			kPrintf("File Read Fail~!!\n");
			fclose(pstFile);
			remove("performance.txt");
			kFreeMemory(pbBuffer);
			return;
		}
	}

	// Å×½ºÆ® ½Ã°£ Ãâ·Â
	kPrintf("    - Read  : %d ms\n", kGetTickCount() - qwLastTickCount);

	//----------------------------------------------------------------------------------------------------
	// 2-1. ¹ÙÀÌÆ® ´ÜÀ§·Î ÆÄÀÏÀ» ¼øÂ÷ÀûÀ¸·Î ¾²´Â Å×½ºÆ®
	//----------------------------------------------------------------------------------------------------
	kPrintf("2. Sequential Read/Write Test (Byte -> 16KB)\n");

	// ±âÁ¸ ÆÄÀÏÀ» »èÁ¦ÇÏ°í »õ·Î »ý¼º
	fclose(pstFile);
	remove("performance.txt");
	pstFile = fopen("performance.txt", "w");
	if(pstFile == NULL){
		kPrintf("File Open Fail~!!\n");
		kFreeMemory(pbBuffer);
		return;
	}

	qwLastTickCount = kGetTickCount();

	// ¹ÙÀÌÆ® ´ÜÀ§·Î ¾²´Â Å×½ºÆ®
	for(i = 0; i < dwOneByteTestFileSize; i++){
		if(fwrite(pbBuffer, 1, 1, pstFile) != 1){
			kPrintf("File Write Fail~!!\n");
			fclose(pstFile);
			remove("performance.txt");
			kFreeMemory(pbBuffer);
			return;
		}
	}

	// Å×½ºÆ® ½Ã°£ Ãâ·Â
	kPrintf("    - Write : %d ms\n", kGetTickCount() - qwLastTickCount);

	//----------------------------------------------------------------------------------------------------
	// 2-2. ¹ÙÀÌÆ® ´ÜÀ§·Î ÆÄÀÏÀ» ¼øÂ÷ÀûÀ¸·Î ÀÐ´Â Å×½ºÆ®
	//----------------------------------------------------------------------------------------------------

	// ÆÄÀÏÀÇ Ã³À½À¸·Î ÀÌµ¿
	fseek(pstFile, 0, SEEK_SET);

	qwLastTickCount = kGetTickCount();

	// ¹ÙÀÌÆ® ´ÜÀ§·Î ÀÐ´Â Å×½ºÆ®
	for(i = 0; i < dwOneByteTestFileSize; i++){
		if(fread(pbBuffer, 1, 1, pstFile) != 1){
			kPrintf("File Read Fail~!!\n");
			fclose(pstFile);
			remove("performance.txt");
			kFreeMemory(pbBuffer);
			return;
		}
	}

	// Å×½ºÆ® ½Ã°£ Ãâ·Â
	kPrintf("    - Read  : %d ms\n", kGetTickCount() - qwLastTickCount);

	// ÆÄÀÏÀ» ´Ý°í, »èÁ¦ ÈÄ, ¸Þ¸ð¸® ÇØÁ¦
	fclose(pstFile);
	remove("performance.txt");
	kFreeMemory(pbBuffer);

	kPrintf("====>>>> File I/O Performance Test End\n");
}

static void kFlushCache(const char* pcParameterBuffer){
	QWORD qwTickCount;

	qwTickCount = kGetTickCount();

	kPrintf("Flush File System Cache...");
	if(kFlushFileSystemCache() == TRUE){
		kPrintf("Success~!!\n");

	}else{
		kPrintf("Fail~!!\n");
	}

	kPrintf("Flush Time = %d ms\n", kGetTickCount() - qwTickCount);
}

static void kDownloadFile(const char* pcParameterBuffer){
	PARAMETERLIST stList;
	char vcFileName[50];
	int iFileNameLength;
	DWORD dwDataLength;
	FILE* fp;
	DWORD dwReceiveSize;
	DWORD dwTempSize;
	BYTE vbDataBuffer[SERIAL_FIFOMAXSIZE];
	QWORD qwLastReceivedTickCount;

	kInitializeParameter(&stList, pcParameterBuffer);
	iFileNameLength = kGetNextParameter(&stList, vcFileName);
	vcFileName[iFileNameLength] = '\0';
	if((iFileNameLength > (FILESYSTEM_MAXFILENAMELENGTH-1)) || (iFileNameLength == 0)){
		kPrintf("Too Long or Too short\n");
		return ;
	}

	// 송수신 fifo 비움
	kClearSerialFIFO();

	// 데이터 길이가 수신될 때까지 기다리는 메시지를 출력하고 4바이트를 수신한 뒤 ack 전송
	kPrintf("wating for data length.......");
	dwReceiveSize = 0;
	qwLastReceivedTickCount = kGetTickCount();
	while(dwReceiveSize<4){
		dwTempSize = kReceivesSerialData(((BYTE*)&dwDataLength)+dwReceiveSize, 4 - dwReceiveSize);
		dwReceiveSize += dwTempSize;

		if(dwTempSize == 0){
			kSleep(0);

			if((kGetTickCount() - qwLastReceivedTickCount) > 30000){
				kPrintf("Time out occur\n");
				return ;
			}
		}
		else
			qwLastReceivedTickCount = kGetTickCount();
	}

	kPrintf("[%d] Byte\n", dwDataLength);
	kSendSerialData("A", 1);

	fp = fopen(vcFileName, "w");
	if(fp == NULL){
		kPrintf("file open fail\n");
		return ;
	}

	kPrintf("Data Recevie Start:");
	dwReceiveSize = 0;
	qwLastReceivedTickCount = kGetTickCount();

	while(dwReceiveSize < dwDataLength){
		dwTempSize = kReceivesSerialData(vbDataBuffer, SERIAL_FIFOMAXSIZE);
		dwReceiveSize += dwTempSize;

		if(dwTempSize != 0){
			if(((dwReceiveSize % SERIAL_FIFOMAXSIZE) == 0) || (dwReceiveSize == dwDataLength)){
				kSendSerialData("A", 1);
				kPrintf("#");
			}

			if(fwrite(vbDataBuffer, 1, dwTempSize, fp) != dwTempSize){
				kPrintf("'%s' file write fail\n");
				break;
			}

			qwLastReceivedTickCount = kGetTickCount();
		}

		else{
			kSleep(0);

			if((kGetTickCount() - qwLastReceivedTickCount) > 10000){
				kPrintf("time out\n");
				return ;
			}
		}
	}

	// 수신 성공 여부 확인후 파일 닫고 파일 시스템 캐시르 ㄹ비움
	if(dwReceiveSize != dwDataLength)
		kPrintf("Receive fail\n");
	else
		kPrintf("Receive success\n");

	fclose(fp);
	kFlushFileSystemCache();
}

static void kShowMPConfigurationTable(const char* pcParameterBuffer){
	kPrintMPConfigurationTable();
}

static void kStartApplicationProcessor(const char* pcParameterBuffer){
	// AP 를깨움
	if(kStartUpApplicationProcessor() == FALSE){
		kPrintf("Application Processor Start Fail\n");
		return ;
	}

	kPrintf("Application Processor Start Success\n");
	kPrintf("BootStrap Processor[APIC ID:%d] Start AP\n", kGetAPICID());
}
