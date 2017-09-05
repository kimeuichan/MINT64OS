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

/***** ���� ���� ���� *****/
// Ŀ�ǵ� ���̺�
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
		{"flush", "Flush File System Cache", kFlushCache}
};

//====================================================================================================
// �� �ڵ� �Լ�
//====================================================================================================
void kStartConsoleShell(void){
	char vcCommandBuffer[CONSOLESHELL_MAXCOMMANDBUFFERCOUNT];
	int iCommandBufferIndex = 0;
	BYTE bKey;
	int iCursorX, iCursorY;

	kPrintf(CONSOLESHELL_PROMPTMESSAGE);

	// �ܼ� �� ���� ����
	while(1){

		// Ű�� ���ŵ� ������ ���
		bKey = kGetCh();

		// BackspaceŰ ó��
		if(bKey == KEY_BACKSPACE){
			if(iCommandBufferIndex > 0){
				kGetCursor(&iCursorX, &iCursorY);
				kPrintStringXY(iCursorX - 1, iCursorY, " ");
				kSetCursor(iCursorX - 1, iCursorY);
				iCommandBufferIndex--;
			}

		// EnterŰ ó��
		}else if(bKey == KEY_ENTER){
			kPrintf("\n");

			// Ŀ�ǵ� ����
			if(iCommandBufferIndex > 0){
				vcCommandBuffer[iCommandBufferIndex] = '\0';
				kExecuteCommand(vcCommandBuffer);
			}

			// ȭ�� �� Ŀ�ǵ� ���� �ʱ�ȭ
			kPrintf("%s", CONSOLESHELL_PROMPTMESSAGE);
			kMemSet(vcCommandBuffer, '\0', CONSOLESHELL_MAXCOMMANDBUFFERCOUNT);
			iCommandBufferIndex = 0;

		// ShiftŰ, Caps LockŰ, Num LockŰ, Scroll LockŰ�� ����
		}else if(bKey == KEY_LSHIFT || bKey == KEY_RSHIFT || bKey == KEY_CAPSLOCK || bKey == KEY_NUMLOCK || bKey == KEY_SCROLLLOCK){
			;

		// �� �� Ű ó��
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

	// �����̽� ��ġ(=Ŀ�ǵ� ����) ���
	for(iSpaceIndex = 0; iSpaceIndex < iCommandBufferLength; iSpaceIndex++){
		if(pcCommandBuffer[iSpaceIndex] == ' '){
			break;
		}
	}

	iCount = sizeof(gs_vstCommandTable) / sizeof(SHELLCOMMANDENTRY);

	for(i = 0; i < iCount; i++){
		iCommandLength = kStrLen(gs_vstCommandTable[i].pcCommand);

		// Ŀ�ǵ��� ���̿� ������ ��ġ�ϴ� ���, Ŀ�ǵ� ó�� �Լ� ȣ��
		if((iCommandLength == iSpaceIndex) && (kMemCmp(gs_vstCommandTable[i].pcCommand, pcCommandBuffer, iSpaceIndex) == 0)){
			gs_vstCommandTable[i].pfFunction(pcCommandBuffer + iSpaceIndex + 1); // Ŀ�ǵ� ó�� �Լ��� ȣ���ϸ鼭, �Ķ���� ����Ʈ ����
			break;
		}
	}

	// Ŀ�ǵ� ���̺� Ŀ�ǵ尡 ���ٸ�, ���� ���
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

	// �����̽� ��ġ(=�Ķ���� ����) ���
	for(i = pstList->iCurrentPosition; i < pstList->iLength; i++){
		if(pstList->pcBuffer[i] == ' '){
			break;
		}
	}

	// �Ķ���� ���� �� ��ġ ����
	kMemCpy(pcParameter, pstList->pcBuffer + pstList->iCurrentPosition, i);
	iLength = i - pstList->iCurrentPosition;
	pcParameter[iLength] = '\0';
	pstList->iCurrentPosition += iLength + 1;

	return iLength; // ���� �Ķ���� ���� ��ȯ
}

//====================================================================================================
// Ŀ�ǵ� ó�� �Լ�
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

	// ���� �� Ŀ�ǵ��� ���̸� ���
	for(i = 0; i < iCount; i++){
		iLength = kStrLen(gs_vstCommandTable[i].pcCommand);
		if(iLength > iMaxCommandLength){
			iMaxCommandLength = iLength;
		}
	}

	// ���� ���
	for(i = 0; i < iCount; i++){
		kPrintf("%s", gs_vstCommandTable[i].pcCommand);
		kGetCursor(&iCursorX, &iCursorY);
		kSetCursor(iMaxCommandLength, iCursorY);
		kPrintf(" - %s\n", gs_vstCommandTable[i].pcHelp);

		// ���� ����� 15�� ��½ø��� ����� �� ������� ���θ� Ȯ��
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
	kSetCursor(0, 1); // ù��° �ٿ��� ���ͷ�Ʈ�� ǥ���ϱ� ������, �ι�° �ٷ� Ŀ�� �̵�
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

	// �Ķ���� �ʱ�ȭ
	kInitializeParameter(&stList, pcParameterBuffer);

	while(1){

		// �Ķ���� ��� : Decimal/Hex String
		iLength = kGetNextParameter(&stList, vcParameter);

		if(iLength == 0){
			break;
		}

		kPrintf("Param %d = '%s', Length = %d, ", iCount + 1, vcParameter, iLength);

		// �Ķ���Ͱ� 16������ ���
		if(kMemCmp(vcParameter, "0x", 2) == 0){
			lValue = kAToI(vcParameter + 2, 16);
			kPrintf("Hex Value = 0x%q\n", lValue); // ��¿� 0x�� �߰�

		// �Ķ���Ͱ� 10������ ���
		}else{
			lValue = kAToI(vcParameter, 10);
			kPrintf("Decimal Value = %d\n", lValue);
		}

		iCount++;
	}
}

static void kShutdown(const char* pcParameterBuffer){
	kPrintf("System Shutdown Start...\n");

	// ���� �ý��� ĳ�� ������ �����͸� �ϵ� ��ũ�� ��
	kPrintf("Flush File System Cache...\n");
	if(kFlushFileSystemCache() == TRUE){
		kPrintf("Success~!!\n");

	}else{
		kPrintf("Fail~!!\n");
	}

	// Ű���� ��Ʈ�ѷ��� ���� PC�� �����
	kPrintf("Press any key to reboot PC...\n");
	kGetCh();
	kReboot();
}

static void kSetTimer(const char* pcParameterBuffer){
	PARAMETERLIST stList;
	char vcParameter[100];
	long lMillisecond;
	BOOL bPeriodic;

	// �Ķ���� �ʱ�ȭ
	kInitializeParameter(&stList, pcParameterBuffer);

	// 1��° �Ķ���� ��� : Millisecond
	if(kGetNextParameter(&stList, vcParameter) == 0){
		kPrintf("Wrong Usage, ex) settimer 1(ms) 1(periodic)\n");
		return;
	}

	lMillisecond = kAToI(vcParameter, 10);

	// 2��° �Ķ���� ��� : Periodic
	if(kGetNextParameter(&stList, vcParameter) == 0){
		kPrintf("Wrong Usage, ex) settimer 1(ms) 1(periodic)\n");
		return;
	}

	bPeriodic = kAToI(vcParameter, 10);

	// PIT �ʱ�ȭ
	kInitializePIT(MSTOCOUNT(lMillisecond), bPeriodic);

	kPrintf("Time = %d ms, Periodic = %d Change Complete\n", lMillisecond, bPeriodic);
}

static void kWaitUsingPIT(const char* pcParameterBuffer){
	char vcParameter[100];
	int iLength;
	PARAMETERLIST stList;
	long lMillisecond;
	int i;

	// �Ķ���� �ʱ�ȭ
	kInitializeParameter(&stList, pcParameterBuffer);

	// 1��° �Ķ���� ��� : Millisecond
	if(kGetNextParameter(&stList, vcParameter) == 0){
		kPrintf("Wrong Usage, ex) wait 1(ms)\n");
		return;
	}

	lMillisecond = kAToI(vcParameter, 10);

	kPrintf("%d ms Sleep Start...\n", lMillisecond);

	// ���ͷ�Ʈ�� ��Ȱ��ȭ�ϰ�, PIT ��Ʈ�ѷ��� ���� ���� �ð��� ����
	kDisableInterrupt();
	for(i = 0; i < (lMillisecond / 30); i++){
		kWaitUsingDirectPIT(MSTOCOUNT(30));
	}
	kWaitUsingDirectPIT(MSTOCOUNT(lMillisecond % 30));
	kEnableInterrupt();

	kPrintf("%d ms Sleep Complete\n", lMillisecond);

	// Ÿ�̸� ����
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

	// 10�� ���� ��ȭ�� Time Stamp Counter�� �̿��Ͽ� ���μ����� �ӵ��� ���������� ����
	kDisableInterrupt();
	for(i = 0; i < 200; i++){
		qwLastTSC = kReadTSC();
		kWaitUsingDirectPIT(MSTOCOUNT(50));
		qwTotalTSC += (kReadTSC() - qwLastTSC);
		kPrintf(".");
	}
	// Ÿ�̸� ����
	kInitializePIT(MSTOCOUNT(1), TRUE);
	kEnableInterrupt();

	kPrintf("\nCPU Speed = %d MHz\n", qwTotalTSC / 10 / 1000 / 1000);
}

static void kShowDateAndTime(const char* pcParameterBuffer){
	WORD wYear;
	BYTE bMonth, bDayOfMonth, bDayOfWeek;
	BYTE bHour, bMinute, bSecond;

	// RTC ��Ʈ�ѷ����� ���� �ð� �� ��¥�� ����
	kReadRTCTime(&bHour, &bMinute, &bSecond);
	kReadRTCDate(&wYear, &bMonth, &bDayOfMonth, &bDayOfWeek);

	kPrintf("Date: %d-%d-%d %d:%d:%d (%s)\n", wYear, bMonth, bDayOfMonth, bHour, bMinute, bSecond, kConvertDayOfWeekToString(bDayOfWeek));
}

// �׽�Ʈ �½�ũ 1 : ȭ�� �׵θ��� ���鼭 ���ڸ� ���
static void kTestTask1(void){
	BYTE bData;
	int i = 0, iX = 0, iY = 0, iMargin, j;
	CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
	TCB* pstRunningTask;

	// TCB ID�� �Ϸ� ��ȣ�� ȭ�� ���������� �̿�
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

		// ���� �κ� �����ٷ�->��Ƽ���� ť �����ٷ��� ���׷��̵��߱� ������, �½�ũ ��ȯ�� �ּ� ó��
		//kSchedule();
	}

	// ���ÿ� ���� �ּҸ� �����߱� ������, �½�ũ ����� �ּ� ó��
	//kExitTask();
}

// �׽�Ʈ �½�ũ 2 : �ڽ��� TCB ID�� �Ϸ� ��ȣ�� �����Ͽ� Ư�� ��ġ�� ȸ���ϴ� �ٶ����� ���
static void kTestTask2(void){
	int i = 0, iOffset;
	CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
	TCB* pstRunningTask;
	char vcData[4] = {'-', '\\', '|', '/'};

	// ���� �½�ũ ID�� �������� ȭ�� ���������� ���
	pstRunningTask = kGetRunningTask();
	iOffset = (pstRunningTask->stLink.qwID & 0xFFFFFFFF) * 2;
	iOffset = (CONSOLE_WIDTH * CONSOLE_HEIGHT) - (iOffset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));

	// ���� ����
	while(1){

		// ȸ���ϴ� �ٶ����� ���
		pstScreen[iOffset].bCharacter = vcData[i % 4];
		pstScreen[iOffset].bAttribute = (iOffset % 15) + 1;
		i++;

		// ���� �κ� �����ٷ�->��Ƽ���� ť �����ٷ��� ���׷��̵��߱� ������, �½�ũ ��ȯ�� �ּ� ó��
		//kSchedule();
	}
}

static void kCreateTestTask(const char* pcParameterBuffer){
	PARAMETERLIST stList;
	char vcParameter[100];
	long lType, lCount;
	int i;

	// �Ķ���� �ʱ�ȭ
	kInitializeParameter(&stList, pcParameterBuffer);

	// 1��° �Ķ���� ��� : Type
	if(kGetNextParameter(&stList, vcParameter) == 0){
		kPrintf("Wrong Usage, ex) createtask 1(type) 1022(count)\n");
		return;
	}

	lType = kAToI(vcParameter, 10);

	// 2��° �Ķ���� ��� : Count
	if(kGetNextParameter(&stList, vcParameter) == 0){
		kPrintf("Wrong Usage, ex) createtask 1(type) 1022(count)\n");
		return;
	}

	lCount = kAToI(vcParameter, 10);

	switch(lType){
	case 1: // �׽�Ʈ �½�ũ 1 : �׵θ� ���� ���
		for(i = 0; i < lCount; i++){
			if(kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kTestTask1) == NULL){
				break;
			}
		}

		kPrintf("kTestTask1 Created(%d)\n", i);
		break;

	case 2: // �׽�Ʈ �½�ũ 2 : �ٶ����� ���
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

	// �Ķ���� �ʱ�ȭ
	kInitializeParameter(&stList, pcParameterBuffer);

	// 1��° �Ķ���� ��� : TaskID
	if(kGetNextParameter(&stList, vcTaskID) == 0){
		kPrintf("Wrong Usage, ex) changepriority 0x300000002(taskId) 0(priority)\n");
		return;
	}

	// 2��° �Ķ���� ��� : Priority
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

	// Task Total Count ���� ä���
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

		// �½�ũ ID�� ���� 32��Ʈ(TCB �Ҵ� Ƚ��)�� 0�� �ƴ��� Ȯ��
		if((pstTCB->stLink.qwID >> 32) != 0){

			// �½�ũ ������ 5�� ��½ø��� ������ �� ������� ���θ� Ȯ��
			if((iCount != 0) && ((iCount % 5) == 0)){

				kPrintf("Press any key to continue...('q' is exit):");

				if(kGetCh() == 'q'){
					kPrintf("\n");
					break;
				}

				kPrintf("\n");
			}

			// �½�ũ ����Ʈ ��� ����
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

	// �Ķ���� �ʱ�ȭ
	kInitializeParameter(&stList, pcParameterBuffer);

	// 1��° �Ķ���� ��� : TaskID
	if(kGetNextParameter(&stList, vcTaskID) == 0){
		kPrintf("Wrong Usage, ex) killtask 0x300000002(taskId) or all\n");
		return;
	}

	// Ư�� ID�� �½�ũ�� ����
	if(kMemCmp(vcTaskID, "all", 3) != 0){

		if(kMemCmp(vcTaskID, "0x", 2) == 0){
			qwTaskID = kAToI(vcTaskID + 2, 16);

		}else{
			qwTaskID = kAToI(vcTaskID, 10);
		}

		// [����]�Ķ���� TaskID�� �½�ũ Ǯ�� ���� TaskID�� ���� �����Ƿ�, �Ķ���� TaskID�� ������(���� 32��Ʈ)������ ����� �Է��ϸ� �ش� �½�ũ�� �����
		pstTCB = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
		qwTaskID = pstTCB->stLink.qwID;

		// �Ҵ�(����)���� ���� �½�ũ�� �ý��� �½�ũ�� ���� ��󿡼� ����
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

	// ��� �½�ũ ����(��, �ܼ� �� �½�ũ, ���� �½�ũ�� ����)
	}else{
		for(i = 0; i < TASK_MAXCOUNT; i++){
			pstTCB = kGetTCBInTCBPool(i);
			qwTaskID = pstTCB->stLink.qwID;

			// �Ҵ�(����)���� ���� �½�ũ�� �ý��� �½�ũ�� ���� ��󿡼� ����
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

/***** ���� ���� ���� *****/
// ���ؽ� �׽�Ʈ�� ���ؽ��� ����
static MUTEX gs_stMutex;
static volatile QWORD gs_qwAdder;

static void kPrintNumberTask(const char* pcParameterBuffer){
	int i, j;
	QWORD qwTickCount;

	// 50ms ���� ����Ͽ�, ���ؽ� �׽�Ʈ�� ��� �޼����� �ܼ� ���� ��� �޼����� ��ġ�� �ʵ��� ��
	qwTickCount = kGetTickCount();
	while((kGetTickCount() - qwTickCount) < 50){
		kSchedule();
	}

	// ���ؽ� �׽�Ʈ�� ���� ���
	for(i = 0; i < 5; i++){
		kLock(&(gs_stMutex));

		kPrintf("Test Mutex : TaskID=[0x%Q] Value=[%d]\n", kGetRunningTask()->stLink.qwID, gs_qwAdder);
		gs_qwAdder++;

		kUnlock(&(gs_stMutex));

		// ���μ����� �Ҹ� �ø����� �߰��� �ڵ�
		for(j = 0; j < 30000; j++);
	}

	// ��� �½�ũ�� ���� ����� �Ϸ��� ������ 1000ms ���� ����Ͽ�, ���ؽ� �׽�Ʈ�� ��� �޼����� ���� �½�ũ�� �½�ũ ���� �޼����� ��ġ�� �ʵ��� ��
	qwTickCount = kGetTickCount();
	while((kGetTickCount() - qwTickCount) < 1000){
		kSchedule();
	}

	// ���ÿ� ���� �ּҸ� �����߱� ������, �½�ũ ����� �ּ� ó��
	// kExitTask();
}

static void kTestMutex(const char* pcParameterBuffer){
	int i;

	gs_qwAdder = 1;

	// ���ؽ� �ʱ�ȭ
	kInitializeMutex(&gs_stMutex);

	// ���ؽ� �׽�Ʈ�� �½�ũ 3�� ����
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

	// ���μ��� 1���� ������ 3���� ����
	pstProcess = kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_PROCESS, (void*)0xEEEEEEEE, 0x1000, (QWORD)kCreateThreadTask);

	if(pstProcess != NULL){
		kPrintf("Process [0x%Q] Create Success\n", pstProcess->stLink.qwID);

	}else{
		kPrintf("Process Create Fail\n");
	}
}

/***** ���� ���� ���� *****/
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

	// Ű�� �ԷµǸ� ���μ��� ����
	kGetCh();
}

static void kShowMatrix(const char* pcParameterBuffer){
	TCB* pstProcess;

	pstProcess = kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_PROCESS, (void*)0xE00000, 0xE00000, (QWORD)kMatrixProcess);

	if(pstProcess != NULL){
		kPrintf("Matrix Process [0x%Q] Create Success\n", pstProcess->stLink.qwID);

		// ���μ����� ����� ������ ���
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

	// ���� �½�ũ ID�� �������� ȭ�� ���������� ���
	pstRunningTask = kGetRunningTask();
	iOffset = (pstRunningTask->stLink.qwID & 0xFFFFFFFF) * 2;
	iOffset = (CONSOLE_WIDTH * CONSOLE_HEIGHT) - (iOffset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));

	// ���� ����
	while(1){
		dValue1 = 1;
		dValue2 = 1;

		// �׽�Ʈ�� ���� ������ ����� 2�� �ݺ��ؼ� ����
		for(i = 0; i < 10; i++){
			qwRandomValue = kRandom();
			dValue1 *= (double)qwRandomValue;
			dValue2 *= (double)qwRandomValue;

			kSleep(1);

			qwRandomValue = kRandom();
			dValue1 /= (double)qwRandomValue;
			dValue2 /= (double)qwRandomValue;

		}

		// FPU ���꿡 ������ �߻��� ���, ���� �޼����� ����ϰ� �½�ũ ����
		if(dValue1 != dValue2){
			kPrintf("Value is not same~!!, [%f] != [%f]\n", dValue1, dValue2);
			break;
		}

		// FPU ���꿡 ������ �߻����� ���� ���, ȸ���ϴ� �ٶ����� ���
		pstScreen[iOffset].bCharacter = vcData[qwCount % 4];
		pstScreen[iOffset].bAttribute = (iOffset % 15) + 1;
		qwCount++;
	}
}

static void kTestPIE(const char* pcParameterBuffer){
	double dResult;
	int i;

	// ���� ��� ��, ���
	kPrintf("PIE Calculation Test\n");
	kPrintf("PIE : 355 / 113 = ");
	dResult = (double)355 / 113;
	//kPrintf("%d.%d%d\n", (QWORD)dResult, ((QWORD)(dResult * 10) % 10), ((QWORD)(dResult * 100) % 10));
	kPrintf("%f\n", dResult);

	// �Ǽ��� ����ϴ� �½�ũ(ȸ���ϴ� �ٶ�����)�� 100�� ����
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

		// ��� ũ���� ����� �Ҵ�޾Ƽ� ���� ä�� �� �˻�
		kPrintf("Allocation and Compare: ");

		for(j = 0; j < (pstMemory->iBlockCountOfSmallestBlock >> i); j++){

			pqwBuffer = (QWORD*)kAllocateMemory(DYNAMICMEMORY_MIN_SIZE << i);
			if(pqwBuffer == NULL){
				kPrintf("\nAllocation Fail~!!\n");
				return;
			}

			// �Ҵ� ���� �޸𸮿� ���� ä��
			for(k = 0; k < ((DYNAMICMEMORY_MIN_SIZE << i) / 8); k++){
				pqwBuffer[k] = k;
			}

			// �˻�
			for(k = 0; k < ((DYNAMICMEMORY_MIN_SIZE << i) / 8); k++){
				if(pqwBuffer[k] != k){
					kPrintf("\nCompare Fail~!!\n");
					return;
				}
			}

			// ��������� .���� ǥ��
			kPrintf(".");
		}

		// �Ҵ���� ����� ��� ����
		kPrintf("\nFree: ");
		for(j = 0; j < (pstMemory->iBlockCountOfSmallestBlock >> i); j++){
			if(kFreeMemory((void*)(pstMemory->qwStartAddress + ((DYNAMICMEMORY_MIN_SIZE << i) * j))) == FALSE){
				kPrintf("\nFree Fail~!!\n");
				return;
			}

			// ��������� .���� ǥ��
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
		// 1KB~32MB ũ���� �޸𸮸� �Ҵ����
		do{
			qwMemorySize = ((kRandom() % (32 * 1024)) + 1) * 1024;
			pbAllocationBuffer = (BYTE*)kAllocateMemory(qwMemorySize);

			// �޸𸮸� �Ҵ���� ������ ���, �ٸ� �½�ũ�� �޸𸮸� ����ϰ� ���� �� �����Ƿ� ��� ����� �� �ٽ� �õ�
			if(pbAllocationBuffer == 0){
				kSleep(1);
			}

		}while(pbAllocationBuffer == 0);

		kSPrintf(vcBuffer, "|Address=[0x%Q], Size=[0x%Q] Allocation Success~!!", pbAllocationBuffer, qwMemorySize);
		kPrintStringXY(20, iY, vcBuffer);
		kSleep(200);

		// ���۸� ������ ������ ������ �����͸� �Ȱ��� ä��
		kSPrintf(vcBuffer, "|Address=[0x%Q], Size=[0x%Q] Data Write...", pbAllocationBuffer, qwMemorySize);
		kPrintStringXY(20, iY, vcBuffer);

		for(i = 0; i < (qwMemorySize / 2); i++){
			pbAllocationBuffer[i] = kRandom() & 0xFF;
			pbAllocationBuffer[i+(qwMemorySize/2)] = pbAllocationBuffer[i];
		}

		kSleep(200);

		// ä�� �����Ͱ� ���������� Ȯ��
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

	// �ϵ� ��ũ ������ ����
	if(kGetHDDInformation(&stHDD) == FALSE){
		kPrintf("HDD Information Read Fail~!!");
		return;
	}

	kPrintf("======================= Primary Master HDD Information ========================\n");

	// �� ��ȣ ���
	kMemCpy(vcBuffer, stHDD.vwModelNumber, sizeof(stHDD.vwModelNumber));
	vcBuffer[sizeof(stHDD.vwModelNumber) - 1] = '\0';
	kPrintf("Model Number   : %s\n", vcBuffer);

	// �ø��� ��ȣ ���
	kMemCpy(vcBuffer, stHDD.vwSerialNumber, sizeof(stHDD.vwSerialNumber));
	vcBuffer[sizeof(stHDD.vwSerialNumber) - 1] = '\0';
	kPrintf("Serial Number  : %s\n", vcBuffer);

	// �Ǹ��� ��, ��� ��, �Ǹ����� ���� �� ���
	kPrintf("Cylinder Count : %d\n", stHDD.wNumberOfCylinder);
	kPrintf("Head Count     : %d\n", stHDD.wNumberOfHead);
	kPrintf("Sector Count   : %d\n", stHDD.wNumberOfSectorPerCylinder);

	// �� ���� �� ���
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

	// �Ķ���� �ʱ�ȭ
	kInitializeParameter(&stList, pcParameterBuffer);

	// 1��° �Ķ���� ��� : LBA
	if(kGetNextParameter(&stList, vcLBA) == 0){
		kPrintf("Wrong Usage, ex) readsector 0(LBA) 10(count)\n");
		return;
	}

	// 2��° �Ķ���� ��� : SectorCount
	if(kGetNextParameter(&stList, vcSectorCount) == 0){
		kPrintf("Wrong Usage, ex) readsector 0(LBA) 10(count)\n");
		return;
	}

	dwLBA = kAToI(vcLBA, 10);
	iSectorCount = kAToI(vcSectorCount, 10);

	// ���� ����ŭ�� �޸𸮸� �Ҵ�
	pcBuffer = (char*)kAllocateMemory(iSectorCount * 512);

	// ���� �б� ����
	if(kReadHDDSector(TRUE, TRUE, dwLBA, iSectorCount, pcBuffer) == iSectorCount){
		kPrintf("HDD Sector Read : LBA=[%d], Count=[%d] : Success~!!", dwLBA, iSectorCount);

		// �޸� ������ ���� ���
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

				// ��� �� �ڸ��� ǥ���ϱ� ����, 16���� ���� ��� 0�� �߰�
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

	// �Ķ���� �ʱ�ȭ
	kInitializeParameter(&stList, pcParameterBuffer);

	// 1��° �Ķ���� ��� : LBA
	if(kGetNextParameter(&stList, vcLBA) == 0){
		kPrintf("Wrong Usage, ex) writesector 0(LBA) 10(count)\n");
		return;
	}

	// 2��° �Ķ���� ��� : SectorCount
	if(kGetNextParameter(&stList, vcSectorCount) == 0){
		kPrintf("Wrong Usage, ex) writesector 0(LBA) 10(count)\n");
		return;
	}

	dwLBA = kAToI(vcLBA, 10);
	iSectorCount = kAToI(vcSectorCount, 10);

	s_dwWriteCount++;

	// ���� ����ŭ�� �޸𸮸� �Ҵ���� ��, �����͸� ä��(������ ������ LBA ��巹��(4 byte)�� ���� ���� Ƚ��(4 byte)�� ����)
	pcBuffer = (char*)kAllocateMemory(iSectorCount * 512);
	for(j = 0; j < iSectorCount; j++){
		for(i = 0; i < 512; i += 8){
			*(DWORD*)&(pcBuffer[j*512+i]) = dwLBA + j;
			*(DWORD*)&(pcBuffer[j*512+i+4]) = s_dwWriteCount;
		}
	}

	// ���� ���� ����
	if(kWriteHDDSector(TRUE, TRUE, dwLBA, iSectorCount, pcBuffer) != iSectorCount){
		kPrintf("HDD Sector Write : Fail~!!\n");
	}

	kPrintf("HDD Sector Write : LBA=[%d], Count=[%d] : Success~!!", dwLBA, iSectorCount);

	// �޸� ������ ���� ���
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

			// ��� �� �ڸ��� ǥ���ϱ� ����, 16���� ���� ��� 0�� �߰�
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

	// �Ķ���� �ʱ�ȭ
	kInitializeParameter(&stList, pcParameterBuffer);

	// 1��° �Ķ���� ��� : FileName
	if((iLength = kGetNextParameter(&stList, vcFileName)) == 0){
		kPrintf("Wrong Usage, ex) createfile a.txt\n");
		return;
	}

	vcFileName[iLength] = '\0';

	if(iLength > (FILESYSTEM_MAXFILENAMELENGTH - 1)){
		kPrintf("Wrong Usage, Too Long File Name\n");
		return;
	}

	// ���� ����
	pstFile = fopen(vcFileName, "w");
	if(pstFile == NULL){
		kPrintf("'%s' File Create Fail~!!\n", vcFileName);
		return;
	}

	// ���� �ݱ�
	fclose(pstFile);
}

static void kDeleteFileInRootDirectory(const char* pcParameterBuffer){
	PARAMETERLIST stList;
	char vcFileName[50];
	int iLength;

	// �Ķ���� �ʱ�ȭ
	kInitializeParameter(&stList, pcParameterBuffer);

	// 1��° �Ķ���� ��� : FileName
	if((iLength = kGetNextParameter(&stList, vcFileName)) == 0){
		kPrintf("Wrong Usage, ex) deletefile a.txt\n");
		return;
	}

	vcFileName[iLength] = '\0';

	if(iLength > (FILESYSTEM_MAXFILENAMELENGTH - 1)){
		kPrintf("Wrong Usage, Too Long File Name\n");
		return;
	}

	// ���� ����
	if(remove(vcFileName) != 0){
		kPrintf("'%s' File Delete Fail~!!\n", vcFileName);
		return;
	}
}

static void kShowRootDirectory(const char* pcParameterBuffer){
	DIR* pstDirectory;
	int i, iCount, iTotalCount;
	struct dirent* pstEntry;
	char vcBuffer[76]; // ���� ũ��(76 byte)�� �ؽ�Ʈ ���� �޸�(ȭ��)�� �� ���� ���̿� ����
	char vcTempValue[50];
	DWORD dwTotalByte;
	DWORD dwUsedClusterCount;
	FILESYSTEMMANAGER stManager;

	// ���� �ý��� ���� ���
	kGetFileSystemInformation(&stManager);

	// ��Ʈ ���丮 ����(��Ʈ ���丮�ۿ� �����Ƿ�, ���丮 �̸�["/"]�� ����)
	pstDirectory = opendir("/");
	if(pstDirectory == NULL){
		kPrintf("Root Directory Open Fail~!!\n");
		return;
	}

	// ��Ʈ ���丮���� �� ���� ����, �� ���� ũ��, ���� Ŭ������ ������ ����
	iTotalCount = 0;
	dwTotalByte = 0;
	dwUsedClusterCount = 0;
	while(1){
		// ��Ʈ ���丮 �б�
		pstEntry = readdir(pstDirectory);
		if(pstEntry == NULL){
			break;
		}

		// �� ���� ����, �� ���� ũ�� ���
		iTotalCount++;
		dwTotalByte += pstEntry->dwFileSize;

		// ���� Ŭ������ ���� ���
		if(pstEntry->dwFileSize == 0){
			// ���� ũ�Ⱑ 0�� ���� Ŭ������ 1���� �Ҵ�Ǿ� ����
			dwUsedClusterCount++;

		}else{
			// ���� ũ�⸦ Ŭ�������� ũ�� ������ ����(�ø� ó��)�Ͽ�, ���� Ŭ������ ������ ����
			dwUsedClusterCount += ((pstEntry->dwFileSize + (FILESYSTEM_CLUSTERSIZE - 1)) / FILESYSTEM_CLUSTERSIZE);
		}
	}

	// ���� ��� ��� ���� ó��
	rewinddir(pstDirectory);
	iCount = 0;
	while(1){
		// ��Ʈ ���丮 �б�
		pstEntry = readdir(pstDirectory);
		if(pstEntry == NULL){
			break;
		}

		// ��� ���۸� �������� �ʱ�ȭ
		kMemSet(vcBuffer, ' ', sizeof(vcBuffer) - 1);
		vcBuffer[sizeof(vcBuffer)-1] = '\0';

		// ��� ���ۿ� ���� �̸� ����
		kMemCpy(vcBuffer, pstEntry->d_name, kStrLen(pstEntry->d_name));

		// ��� ���ۿ� ���� ũ�� ����
		kSPrintf(vcTempValue, "%d byte", pstEntry->dwFileSize);
		kMemCpy(vcBuffer + 30, vcTempValue, kStrLen(vcTempValue));

		// ��� ���ۿ� ���� Ŭ������ �ε��� ����
		kSPrintf(vcTempValue, "0x%X cluster", pstEntry->dwStartClusterIndex);
		kMemCpy(vcBuffer + 55, vcTempValue, kStrLen(vcTempValue));

		// ���� ��� ���
		kPrintf("    %s\n", vcBuffer);

		// ���� ����� 15�� ��½ø��� ����� �� ������� ���θ� Ȯ��
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

	// �� ���� ����, �� ���� ũ��, �ϵ� ��ũ�� ���� ����(���� ����)�� ���
	kPrintf("\t\tTotal File Count : %d\n", iTotalCount);
	kPrintf("\t\tTotal File Size  : %d byte (%d cluster)\n", dwTotalByte, dwUsedClusterCount);
	kPrintf("\t\tFree Space       : %d KB (%d cluster)\n", (stManager.dwTotalClusterCount - dwUsedClusterCount) * FILESYSTEM_CLUSTERSIZE / 1024, stManager.dwTotalClusterCount - dwUsedClusterCount);

	// ��Ʈ ���丮 �ݱ�
	closedir(pstDirectory);
}

static void kWriteDataToFile(const char* pcParameterBuffer){
	PARAMETERLIST stList;
	char vcFileName[50];
	int iLength;
	FILE* fp;
	int iEnterCount;
	BYTE bKey;

	// �Ķ���� �ʱ�ȭ
	kInitializeParameter(&stList, pcParameterBuffer);

	// 1��° �Ķ���� ��� : FileName
	if((iLength = kGetNextParameter(&stList, vcFileName)) == 0){
		kPrintf("Wrong Usage, ex) writefile a.txt\n");
		return;
	}

	vcFileName[iLength] = '\0';

	if(iLength > (FILESYSTEM_MAXFILENAMELENGTH - 1)){
		kPrintf("Wrong Usage, Too Long File Name\n");
		return;
	}

	// ���� ����
	fp = fopen(vcFileName, "w");
	if(fp == NULL){
		kPrintf("'%s' File Open Fail~!!\n", vcFileName);
		return;
	}

	// ���� ���� ����
	iEnterCount = 0;
	while(1){
		bKey = kGetCh();

		// ���� Ű�� 3�� ���� ������ ���, ����
		if(bKey == KEY_ENTER){
			iEnterCount++;
			if(iEnterCount >= 3){
				break;
			}
		}else{
			iEnterCount = 0;
		}

		kPrintf("%c", bKey);

		// ���� ����
		if(fwrite(&bKey, 1, 1, fp) != 1){
			kPrintf("'%s' File Write Fail~!!\n", vcFileName);
			break;
		}
	}

	// ���� �ݱ�
	fclose(fp);
}

static void kReadDataFromFile(const char* pcParameterBuffer){
	PARAMETERLIST stList;
	char vcFileName[50];
	int iLength;
	FILE* fp;
	int iEnterCount;
	BYTE bKey;

	// �Ķ���� �ʱ�ȭ
	kInitializeParameter(&stList, pcParameterBuffer);

	// 1��° �Ķ���� ��� : FileName
	if((iLength = kGetNextParameter(&stList, vcFileName)) == 0){
		kPrintf("Wrong Usage, ex) readfile a.txt\n");
		return;
	}

	vcFileName[iLength] = '\0';

	if(iLength > (FILESYSTEM_MAXFILENAMELENGTH - 1)){
		kPrintf("Wrong Usage, Too Long File Name\n");
		return;
	}

	// ���� ����
	fp = fopen(vcFileName, "r");
	if(fp == NULL){
		kPrintf("'%s' File Open Fail~!!\n", vcFileName);
		return;
	}

	// ���� �б� ����
	iEnterCount = 0;
	while(1){
		// ���� �б�
		if(fread(&bKey, 1, 1, fp) != 1){
			break;
		}

		kPrintf("%c", bKey);

		// ���� Ű�� ���, Ƚ�� ����
		if(bKey == KEY_ENTER){
			iEnterCount++;

			// ���� ������ 15���� ��½ø��� ������ �� ������� ���θ� Ȯ��
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

	// ���� �ݱ�
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

	// 4MB �޸�(����) �Ҵ�
	dwMaxFileSize = 4 * 1024 * 1024;
	pbBuffer = (BYTE*)kAllocateMemory(dwMaxFileSize);
	if(pbBuffer == NULL){
		kPrintf("Memory Allocation Fail~!!\n");
		return;
	}

	// �׽�Ʈ�� ���� ����
	remove("testfileio.bin");

	//----------------------------------------------------------------------------------------------------
	// 1. ���� ���� ���� �׽�Ʈ
	//----------------------------------------------------------------------------------------------------
	kPrintf("1. File Open Fail Test...");

	// �б� ���(r)�� ������ �������� �ʴ� ��� ������ �������� �����Ƿ�, NULL�� ����
	pstFile = fopen("testfileio.bin", "r");
	if(pstFile == NULL){
		kPrintf("[Pass]\n");

	}else{
		kPrintf("[Fail]\n");
		fclose(pstFile);
	}

	//----------------------------------------------------------------------------------------------------
	// 2. ���� ���� �׽�Ʈ
	//----------------------------------------------------------------------------------------------------
	kPrintf("2. File Create Test...");

	// ���� ���(w)�� ������ �������� �ʴ� ��� ������ �����ϹǷ�, ���� �ڵ��� ����
	pstFile = fopen("testfileio.bin", "w");
	if(pstFile != NULL){
		kPrintf("[Pass]\n");
		kPrintf("    FileHandle=[0x%Q]\n", pstFile);

	}else{
		kPrintf("[Fail]\n");
	}

	//----------------------------------------------------------------------------------------------------
	// 3. �������� ���� ���� �׽�Ʈ
	//----------------------------------------------------------------------------------------------------
	kPrintf("3. Sequential Write Test(Cluster Size)...");

	// ���� ���Ͽ� ���� ����
	for(i = 0; i < 100; i++){

		kMemSet(pbBuffer, i, FILESYSTEM_CLUSTERSIZE);

		// ���� ����
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
	// 4. �������� ���� �б� �� ���� �׽�Ʈ
	//----------------------------------------------------------------------------------------------------
	kPrintf("4. Sequential Read and Verify Test(Cluster Size)...");

	// ������ ó������ �̵�
	fseek(pstFile, -100 * FILESYSTEM_CLUSTERSIZE, SEEK_END);

	// ���� ���Ͽ��� �б� ���� �� ������ ����
	for(i = 0; i < 100; i++){

		// ���� �б�
		if(fread(pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile) != FILESYSTEM_CLUSTERSIZE){
			kPrintf("[Fail]\n");
			break;
		}

		// ������ ����
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
	// 5. ������ ���� ���� �׽�Ʈ
	//----------------------------------------------------------------------------------------------------
	kPrintf("5. Random Write Test\n");

	// ���۸� 0���� ä��
	kMemSet(pbBuffer, 0, dwMaxFileSize);

	// ������ ó������ �̵� ��, ������ ������ �о ���۷� ����
	fseek(pstFile, -100 * FILESYSTEM_CLUSTERSIZE, SEEK_CUR);
	fread(pbBuffer, 1, dwMaxFileSize, pstFile);

	// ���ϰ� ������ ������ ��ġ�� ������ �����͸� ��
	for(i = 0; i < 100; i++){
		dwByteCount = (kRandom() % (sizeof(vbTempBuffer) - 1)) + 1;
		dwRandomOffset = kRandom() % (dwMaxFileSize - dwByteCount);
		kPrintf("    [%d] Offset=[%d], Byte=[%d]...", i, dwRandomOffset, dwByteCount);

		// ������ ������ ��ġ�� ����
		fseek(pstFile, dwRandomOffset, SEEK_SET);
		kMemSet(vbTempBuffer, i, dwByteCount);
		if(fwrite(vbTempBuffer, 1, dwByteCount, pstFile) != dwByteCount){
			kPrintf("[Fail]\n");
			break;

		}else{
			kPrintf("[Pass]\n");
		}

		// ������ ������ ��ġ�� ����
		kMemSet(pbBuffer + dwRandomOffset, i, dwByteCount);
	}

	// ���ϰ� ������ ���������� �̵��Ͽ� 1����Ʈ�� �Ἥ, ũ�⸦ 4MB�� ����
	fseek(pstFile, dwMaxFileSize - 1, SEEK_SET);
	fwrite(&i, 1, 1, pstFile);
	pbBuffer[dwMaxFileSize - 1] = (BYTE)i;

	//----------------------------------------------------------------------------------------------------
	// 6. ������ ���� �б� �� ���� �׽�Ʈ
	//----------------------------------------------------------------------------------------------------
	kPrintf("6. Random Read and Verify Test\n");

	// ���ϰ� ������ ������ ��ġ���� �����͸� ���� ��, ������ ����
	for(i = 0; i < 100; i++){
		dwByteCount = (kRandom() % (sizeof(vbTempBuffer) - 1)) + 1;
		dwRandomOffset = kRandom() % (dwMaxFileSize - dwByteCount);
		kPrintf("    [%d] Offset=[%d], Byte=[%d]...", i, dwRandomOffset, dwByteCount);

		// ������ ������ ��ġ���� �б�
		fseek(pstFile, dwRandomOffset, SEEK_SET);
		if(fread(vbTempBuffer, 1, dwByteCount, pstFile) != dwByteCount){
			kPrintf("[Fail]\n");
			kPrintf("    File Read Fail\n");
			break;
		}

		// ������ ����
		if(kMemCmp(pbBuffer + dwRandomOffset, vbTempBuffer, dwByteCount) != 0){
			kPrintf("[Fail]\n");
			kPrintf("    Data Verify Fail\n");
			break;
		}

		kPrintf("[Pass]\n");
	}

	//----------------------------------------------------------------------------------------------------
	// 7. �������� ���� ����, �б� �� ���� �׽�Ʈ
	//----------------------------------------------------------------------------------------------------
	kPrintf("7. Sequential Write, Read and Verify Test(1024 byte)\n");

	// ������ ó������ �̵�
	fseek(pstFile, -dwMaxFileSize, SEEK_CUR);

	// ���� ���� ����(�պκп� 2MB�� ��)
	for(i = 0; i < (2 * 1024 * 1024 / 1024); i++){
		kPrintf("    [%d] Offset=[%d], Byte=[%d] Write...", i, i * 1024, 1024);

		// ���� ����(1024 byte�� ��)
		if(fwrite(pbBuffer + (i * 1024), 1, 1024, pstFile) != 1024){
			kPrintf("[Fail]\n");
			break;

		}else{
			kPrintf("[Pass]\n");
		}
	}

	// ������ ó������ �̵�
	fseek(pstFile, -dwMaxFileSize, SEEK_SET);

	// ���� �б� ��, ������ ����(Random Write�� �����Ͱ� �߸� ����Ǿ��� ���� �����Ƿ�, ������ 4MB ��ü ������ ������� ��)
	for(i = 0; i < (dwMaxFileSize / 1024); i++){
		kPrintf("    [%d] Offset=[%d], Byte=[%d] Read and Verify...", i, i * 1024, 1024);

		// ���� �б�(1024 byte�� ����)
		if(fread(vbTempBuffer, 1, 1024, pstFile) != 1024){
			kPrintf("[Fail]\n");
			break;
		}

		// ������ ����
		if(kMemCmp(pbBuffer + (i * 1024), vbTempBuffer, 1024) != 0){
			kPrintf("[Fail]\n");
			break;

		}else{
			kPrintf("[Pass]\n");
		}
	}

	//----------------------------------------------------------------------------------------------------
	// 8. ���� ���� ���� �׽�Ʈ
	//----------------------------------------------------------------------------------------------------
	kPrintf("8. File Delete Fail Test...");

	// ���� ������ ���� �ִ� �����̹Ƿ�, ���� ������ �����ؾ� ��
	if(remove("testfileio.bin") != 0){
		kPrintf("[Pass]\n");

	}else{
		kPrintf("[Fail]\n");
	}

	//----------------------------------------------------------------------------------------------------
	// 9. ���� �ݱ� �׽�Ʈ
	//----------------------------------------------------------------------------------------------------
	kPrintf("9. File Close Fail Test...");

	// ���� �ݱ�
	if(fclose(pstFile) == 0){
		kPrintf("[Pass]\n");

	}else{
		kPrintf("[Fail]\n");
	}

	//----------------------------------------------------------------------------------------------------
	// 10. ���� ���� �׽�Ʈ
	//----------------------------------------------------------------------------------------------------
	kPrintf("10. File Delete Test...");

	// ���� ����
	if(remove("testfileio.bin") == 0){
		kPrintf("[Pass]\n");

	}else{
		kPrintf("[Fail]\n");
	}

	// �޸�(����) ����
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

	// ���� ũ�� ����(Ŭ������ ����:1MB, ����Ʈ ����:16KB)
	dwClusterTestFileSize = 1024 * 1024;
	dwOneByteTestFileSize = 16 * 1024;

	// �޸� �Ҵ�
	pbBuffer = (BYTE*)kAllocateMemory(dwClusterTestFileSize);
	if(pbBuffer == NULL){
		kPrintf("Memory Allocate Fail~!!\n");
		return;
	}

	kMemSet(pbBuffer, 0, FILESYSTEM_CLUSTERSIZE);

	kPrintf("====>>>> File I/O Performance Test Start\n");

	//----------------------------------------------------------------------------------------------------
	// 1-1. Ŭ������ ������ ������ ���������� ���� �׽�Ʈ
	//----------------------------------------------------------------------------------------------------
	kPrintf("1. Sequential Read/Write Test (Cluster -> 1MB)\n");

	// ���� ������ �����ϰ� ���� ����
	remove("performance.txt");
	pstFile = fopen("performance.txt", "w");
	if(pstFile == NULL){
		kPrintf("File Open Fail~!!\n");
		kFreeMemory(pbBuffer);
		return;
	}

	qwLastTickCount = kGetTickCount();

	// Ŭ������ ������ ���� �׽�Ʈ
	for(i = 0; i < (dwClusterTestFileSize / FILESYSTEM_CLUSTERSIZE); i++){
		if(fwrite(pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile) != FILESYSTEM_CLUSTERSIZE){
			kPrintf("File Write Fail~!!\n");
			fclose(pstFile);
			remove("performance.txt");
			kFreeMemory(pbBuffer);
			return;
		}
	}

	// �׽�Ʈ �ð� ���
	kPrintf("    - Write : %d ms\n", kGetTickCount() - qwLastTickCount);

	//----------------------------------------------------------------------------------------------------
	// 1-2. Ŭ������ ������ ������ ���������� �д� �׽�Ʈ
	//----------------------------------------------------------------------------------------------------

	// ������ ó������ �̵�
	fseek(pstFile, 0, SEEK_SET);

	qwLastTickCount = kGetTickCount();

	// Ŭ������ ������ �д� �׽�Ʈ
	for(i = 0; i < (dwClusterTestFileSize / FILESYSTEM_CLUSTERSIZE); i++){
		if(fread(pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile) != FILESYSTEM_CLUSTERSIZE){
			kPrintf("File Read Fail~!!\n");
			fclose(pstFile);
			remove("performance.txt");
			kFreeMemory(pbBuffer);
			return;
		}
	}

	// �׽�Ʈ �ð� ���
	kPrintf("    - Read  : %d ms\n", kGetTickCount() - qwLastTickCount);

	//----------------------------------------------------------------------------------------------------
	// 2-1. ����Ʈ ������ ������ ���������� ���� �׽�Ʈ
	//----------------------------------------------------------------------------------------------------
	kPrintf("2. Sequential Read/Write Test (Byte -> 16KB)\n");

	// ���� ������ �����ϰ� ���� ����
	fclose(pstFile);
	remove("performance.txt");
	pstFile = fopen("performance.txt", "w");
	if(pstFile == NULL){
		kPrintf("File Open Fail~!!\n");
		kFreeMemory(pbBuffer);
		return;
	}

	qwLastTickCount = kGetTickCount();

	// ����Ʈ ������ ���� �׽�Ʈ
	for(i = 0; i < dwOneByteTestFileSize; i++){
		if(fwrite(pbBuffer, 1, 1, pstFile) != 1){
			kPrintf("File Write Fail~!!\n");
			fclose(pstFile);
			remove("performance.txt");
			kFreeMemory(pbBuffer);
			return;
		}
	}

	// �׽�Ʈ �ð� ���
	kPrintf("    - Write : %d ms\n", kGetTickCount() - qwLastTickCount);

	//----------------------------------------------------------------------------------------------------
	// 2-2. ����Ʈ ������ ������ ���������� �д� �׽�Ʈ
	//----------------------------------------------------------------------------------------------------

	// ������ ó������ �̵�
	fseek(pstFile, 0, SEEK_SET);

	qwLastTickCount = kGetTickCount();

	// ����Ʈ ������ �д� �׽�Ʈ
	for(i = 0; i < dwOneByteTestFileSize; i++){
		if(fread(pbBuffer, 1, 1, pstFile) != 1){
			kPrintf("File Read Fail~!!\n");
			fclose(pstFile);
			remove("performance.txt");
			kFreeMemory(pbBuffer);
			return;
		}
	}

	// �׽�Ʈ �ð� ���
	kPrintf("    - Read  : %d ms\n", kGetTickCount() - qwLastTickCount);

	// ������ �ݰ�, ���� ��, �޸� ����
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

