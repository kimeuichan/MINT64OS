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

/***** 전역 변수 정의 *****/
// 커맨드 테이블
static SHELLCOMMANDENTRY gs_vstCommandTable[] = {
		{"help", "Show Help", kHelp},
		{"cls", "Clear Screen", kCls},
		{"totalram", "Show Total RAM Size", kShowTotalRAMSize},
		{"strtod", "String to Decimal/Hex Convert", kStringToDecimalHexTest},
		{"shutdown", "Shutdown and Reboot OS", kShutdown},
		{"settimer", "Set PIT Contoller Counter0, ex)settimer 10(ms) 1(periodic)", kSetTimer},
		{"wait", "Wait ms Using PIT, ex)wait 100(ms)", kWaitUsingPIT},
		{"rdtsc", "Read Time Stam Counter", kReadTimeStampCounter},
		{"cpuspeed", "Measure Processor Speed", kMeasureProcessorSpeed},
		{"date", "Show Date And Time", kShowDateAndTime},
		{"createtask", "Create Task, ex)createtask 1(type) 10(count)", kCreateTestTask},
		{"changepriority", "Change Task Priority, ex)changepriority 1(ID) 2(Priority)", 
			kChangeTaskPriority},
		{"tasklist", "Show Task List", kShowTaskList},
		{"killtask", "End Task", kKillTask},
		{"cpuload", "Show Processor Load", kCPULoad},
		{"testmutex", "Test Mutex Function", kTestMutex},
		{"testthread", "Test Thread And Process Function", kTestThread},
		{"showmatrix", "Show Matrix Screen", kShowMatrix},
		{"testpie", "Test Pie calculation", kTestPIE},
		{"dynamicmeminfo", "Show Total RAM Size", kShowDynamicMemoryInformation},
		{"testseqalloc", "Test Sequential Allocation & Free", kTestSequentialAllocation},
		{"testranalloc", "Test Random Allocation & Free", kTestRandomAllocation},
		{"hddinfo", "Show Hdd Information", kShowHDDInformation},
		{"readsector", "Read HDD Sector, ex)readsector 0(LBA) 10(count)", kReadSector},
		{"writesector", "Write HDD sector, ex)writesector 0(LBA) 10(count)", kWriteSector},
		{"mounthdd", "Mount HDD", kMountHDD},
		{"formathdd", "Format HDD", kFormatHDD},
		{"filesysteminfo", "Show File System Information", kShowFileSystemInformation},
		{"createfile", "Create File ex)createfile a.txt", kCreateFileInRootDirectory},
		{"deletefile", "Delete File ex)deletefile a.txt", kDeleteFileInRootDirectory},
		{"dir", "Show Directory", kShowRootDirectory},

};

//====================================================================================================
// 쉘 코드 함수
//====================================================================================================
void kStartConsoleShell(void){
	char vcCommandBuffer[CONSOLESHELL_MAXCOMMANDBUFFERCOUNT];
	int iCommandBufferIndex = 0;
	BYTE bKey;
	int iCursorX, iCursorY;

	kPrintf(CONSOLESHELL_PROMPTMESSAGE);

	// 콘솔 쉘 메인 루프
	while(1){

		// 키가 수신될 때까지 대기
		bKey = kGetCh();

		// Backspace키 처리
		if(bKey == KEY_BACKSPACE){
			if(iCommandBufferIndex > 0){
				kGetCursor(&iCursorX, &iCursorY);
				kPrintStringXY(iCursorX - 1, iCursorY, " ");
				kSetCursor(iCursorX - 1, iCursorY);
				iCommandBufferIndex--;
			}

		// Enter키 처리
		}else if(bKey == KEY_ENTER){
			kPrintf("\n");

			// 커맨드 실행
			if(iCommandBufferIndex > 0){
				vcCommandBuffer[iCommandBufferIndex] = '\0';
				kExecuteCommand(vcCommandBuffer);
			}

			// 화면 및 커맨드 버퍼 초기화
			kPrintf("%s", CONSOLESHELL_PROMPTMESSAGE);
			kMemSet(vcCommandBuffer, '\0', CONSOLESHELL_MAXCOMMANDBUFFERCOUNT);
			iCommandBufferIndex = 0;

		// Shift키, Caps Lock키, Num Lock키, Scroll Lock키는 무시
		}else if(bKey == KEY_LSHIFT || bKey == KEY_RSHIFT || bKey == KEY_CAPSLOCK || bKey == KEY_NUMLOCK || bKey == KEY_SCROLLLOCK){
			;

		// 그 외 키 처리
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

	// 스페이스 위치(=커맨드 길이) 취득
	for(iSpaceIndex = 0; iSpaceIndex < iCommandBufferLength; iSpaceIndex++){
		if(pcCommandBuffer[iSpaceIndex] == ' '){
			break;
		}
	}

	iCount = sizeof(gs_vstCommandTable) / sizeof(SHELLCOMMANDENTRY);

	for(i = 0; i < iCount; i++){
		iCommandLength = kStrLen(gs_vstCommandTable[i].pcCommand);

		// 커맨드의 길이와 내용이 일치하는 경우, 커맨드 처리 함수 호출
		if((iCommandLength == iSpaceIndex) && (kMemCmp(gs_vstCommandTable[i].pcCommand, pcCommandBuffer, iSpaceIndex) == 0)){
			gs_vstCommandTable[i].pfFunction(pcCommandBuffer + iSpaceIndex + 1); // 커맨드 처리 함수를 호출하면서, 파라미터 리스트 전달
			break;
		}
	}

	// 커맨드 테이블에 커맨드가 없다면, 에러 출력
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

	// 스페이스 위치(=파라미터 길이) 취득
	for(i = pstList->iCurrentPosition; i < pstList->iLength; i++){
		if(pstList->pcBuffer[i] == ' '){
			break;
		}
	}

	// 파라미터 복사 및 위치 갱신
	kMemCpy(pcParameter, pstList->pcBuffer + pstList->iCurrentPosition, i);
	iLength = i - pstList->iCurrentPosition;
	pcParameter[iLength] = '\0';
	pstList->iCurrentPosition += iLength + 1;

	return iLength; // 현재 파라미터 길이 반환
}

//====================================================================================================
// 커맨드 처리 함수
//====================================================================================================
static void kHelp(const char* pcParameterBuffer){
	int i;
	int iCount;
	int iCursorX, iCursorY;
	int iLength, iMaxCommandLength = 0;

	kPrintf("==================================================\n");
	kPrintf("               MINT64 Shell Help                  \n");
	kPrintf("==================================================\n");

	iCount = sizeof(gs_vstCommandTable) / sizeof(SHELLCOMMANDENTRY);

	kPrintf("count = %d\n", iCount);

	// 가장 긴 커맨드의 길이를 계산
	for(i = 0; i < iCount; i++){
		iLength = kStrLen(gs_vstCommandTable[i].pcCommand);
		if(iLength > iMaxCommandLength){
			iMaxCommandLength = iLength;
		}
	}

	// 도움말 출력
	for(i = 0; i < iCount; i++){
		kPrintf("%s", gs_vstCommandTable[i].pcCommand);
		kGetCursor(&iCursorX, &iCursorY);
		kSetCursor(iMaxCommandLength, iCursorY);
		kPrintf(" - %s\n", gs_vstCommandTable[i].pcHelp);

		// 목록이 많을 경우
		if( (i != 0) && ((i % 20)) == 0){
			kPrintf("Press any key to continue ... ('q' is exist) :");
			if(kGetCh() == 'q'){
				kPrintf("\n");
				break;
			}
			kPrintf("\n");
		}
	}
}

static void kCls(const char* pcParameterBuffer){
	kClearScreen();
	kSetCursor(0, 1); // 첫번째 줄에는 인터럽트를 표시하기 때문에, 두번째 줄로 커서 이동
}

static void kShowTotalRAMSize(const char* pcParameterBuffer){
	kPrintf("Total RAM Size = %d MB\n", kGetTotalRAMSize());
}

static void kStringToDecimalHexTest(const char* pcParameterBuffer){
	char vcParameter[100];
	int iLength;
	PARAMETERLIST stList;
	int iCount = 0;
	long lValue;

	kInitializeParameter(&stList, pcParameterBuffer);

	while(1){
		iLength = kGetNextParameter(&stList, vcParameter);

		if(iLength == 0){
			break;
		}

		kPrintf("Param %d = '%s', Length = %d, ", iCount + 1, vcParameter, iLength);

		// 파라미터가 16진수인 경우
		if(kMemCmp(vcParameter, "0x", 2) == 0){
			lValue = kAToI(vcParameter + 2, 16);
			kPrintf("Hex Value = 0x%q\n", lValue); // 출력에 0x를 추가

		// 파라미터가 10진수인 경우
		}else{
			lValue = kAToI(vcParameter, 10);
			kPrintf("Decimal Value = %d\n", lValue);
		}

		iCount++;
	}
}

static void kShutdown(const char* pcParameterBuffer){
	kPrintf("System Shutdown Start...\n");

	// 키보드 컨트롤러를 통해 PC를 재부팅
	kPrintf("Press any key to reboot PC...");
	kGetCh();
	kReboot();
}

static void kSetTimer(const char* pcParameterBuffer){
	char vcParameter[100];
	PARAMETERLIST stList;
	long lValue;
	BOOL bPeriodic;

	kInitializeParameter(&stList, pcParameterBuffer);

	if(kGetNextParameter(&stList, vcParameter) == 0){
		kPrintf("ex)settimer 10(ms) 1(periodic)\n");
		return;
	}
	lValue = kAToI(vcParameter, 10);

	if(kGetNextParameter(&stList, vcParameter) == 0){
		kPrintf("ex)settimer 10(ms) 1(periodic)\n");
		return;
	}

	bPeriodic = kAToI(vcParameter, 10);

	kInitializePIT(MSTOCOUNT(lValue), bPeriodic);
	kPrintf("Time = %d ms, Periodic = %d Change Complete\n", lValue, bPeriodic);
}

static void kWaitUsingPIT(const char* pcParameterBuffer){
	char vcParameter[100];
	int iLength;
	PARAMETERLIST stList;
	long lMillisecond;
	int i;

	kInitializeParameter(&stList, pcParameterBuffer);
	if(kGetNextParameter(&stList, vcParameter) == 0){
		kPrintf("ex)wait 100(ms)\n");
		return;
	}

	lMillisecond = kAToI(vcParameter, 10);
	kPrintf("%d ms Sleep Start..\n", lMillisecond);

	// 인터럽트를 비활성화 하고 PIT 컨트롤럴 통해 직접 시간을 측정
	kDisableInterrupt();
	for(i=0; i< (lMillisecond / 30); i++){
		kWaitUsingDirectPIT(MSTOCOUNT(30));
	}
	kWaitUsingDirectPIT(MSTOCOUNT(lMillisecond % 30));
	kEnableInterrupt();
	kPrintf("%d ms Sleep Complete\n", lMillisecond);

	// 타이머 복원
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

	kPrintf("NOw Measuring.");

	// 10초 동안 변화한 타임 스태브 카운터를 이용하여 프로세서의 속도를 간접적으로 측정
	kDisableInterrupt();
	for(i=0; i<200; i++){
		qwLastTSC = kReadTSC();
		kWaitUsingDirectPIT(MSTOCOUNT(50));
		qwTotalTSC += (kReadTSC() - qwLastTSC);

		kPrintf(".");
	}
	// 타이머 복원
	kInitializePIT(MSTOCOUNT(1), TRUE);
	kEnableInterrupt();

	kPrintf("\nCPU Speed = %d MHz\n", qwTotalTSC / 10 / 1000/ 1000);
}

// RTC 컨트롤러에 저장된 일자 및 시간 표시
static void kShowDateAndTime(const char* pcParameterBuffer){
	BYTE bSecond, bMinute, bHour;
	BYTE bDayOfWeek, bDayOfMonth, bMonth;
	WORD wYear;

	// RTC 컨트롤러에서 시간 및 일자를 읽음
	kReadRTCTime(&bHour, &bMinute, &bSecond);
	kReadRTCDate(&wYear, &bMonth, &bDayOfMonth, &bDayOfWeek);

	kPrintf("Date : %d/%d/%d %s, ", wYear, bMonth, bDayOfMonth, 
		kConvertDayOfWeekToString(bDayOfWeek));
	kPrintf("Time : %d:%d:%d\n", bHour, bMinute, bSecond);
}

// TCB 자료구조와 스택 정의
static TCB gs_vstTask[2] = {0,};
static QWORD gs_vstStack[1024] = {0,};

static void kTestTask(void){
	int i = 0;
	while(1){
		kPrintf("[%d] This message is from kTestTask, Press any key to siwtch kConsoleShell~!!\n", i++);
		kGetCh();

		kSwitchContext( &(gs_vstTask[1].stContext), &(gs_vstTask[0].stContext));
	}
}

// 태스크1
//	화면 테두리를 돌면서 문자를 출력
static void kTestTask1(void){
	BYTE bData;
	int i = 0, iX = 0, iY = 0, iMargin, j;
	CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
	TCB* pstRunningTask;

	// 자신의 ID 얻어서 화면 오프셋으로 사용
	pstRunningTask = kGetRunningTask();
	iMargin = (pstRunningTask->stLink.qwID & 0xffffffff) % 10;

	for(j=0; j<20000; j++){
		switch(i){
			case 0:
				iX++;
				if(iX>=(CONSOLE_WIDTH - iMargin))
					i = 1;
				break;
			case 1:
				iY++;
				if(iY >= (CONSOLE_HEIGHT - iMargin))
					i = 2;
				break;
			case 2:
				iX--;
				if(iX < iMargin)
					i = 3;
				break;
			case 3:
				iY--;
				if(iY < iMargin)
					i = 0;
				break;
		}

		pstScreen[iY*CONSOLE_WIDTH + iX].bCharacter = bData;
		pstScreen[iY*CONSOLE_WIDTH + iX].bAttribute = bData & 0x0f;
		bData++;

		//kSchedule();
	}
	kExitTask();
}

// 태스크2
//	자신의 ID를 참고하여 특정 위치에 회전하는 바람개비를 출력
static void kTestTask2(void){
	int i = 0, iOffset;
	CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
	TCB* pstRunningTask;
	char vcData[4] = {'-', '\\', '|', '/'};

	// TCB ID의 일련 번호를 화면 오프셋으로 이용
	pstRunningTask = kGetRunningTask();
	iOffset = (pstRunningTask->stLink.qwID & 0xFFFFFFFF) * 2;
	iOffset = (CONSOLE_WIDTH * CONSOLE_HEIGHT) - (iOffset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));

	while(1){
		pstScreen[iOffset].bCharacter = vcData[i % 4];
		pstScreen[iOffset].bAttribute = (iOffset % 15) + 1;
		i++;

		// 태스크 전환
		kSchedule();
	}
}

static void kCreateTestTask(const char* pcParameterBuffer){
	PARAMETERLIST stList;
	char vcType[30];
	char vcCount[30];
	int i;

	kInitializeParameter(&stList, pcParameterBuffer);
	kGetNextParameter(&stList, vcType);
	kGetNextParameter(&stList, vcCount);

	switch(kAToI(vcType, 10)){
		case 1:
			for(i=0; i<kAToI(vcCount, 10); i++){
				if(kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kTestTask1) == NULL)
					break;
			}
			kPrintf("Task1 %d Created\n", i);
			break;
		case 2:
			for(i=0; i<kAToI(vcCount, 10); i++){
				if(kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kTestTask2) == NULL)
					break;
			}
			kPrintf("Task2 %d Created\n", i);
			break;
	}
	
}

static void kChangeTaskPriority(const char* pcParameterBuffer){
	PARAMETERLIST stList;
	char vcID[30];
	char vcPriority[30];
	QWORD qwID;
	BYTE bPriority;

	kInitializeParameter(&stList, pcParameterBuffer);
	kGetNextParameter(&stList, vcID);
	kGetNextParameter(&stList, vcPriority);

	// 태스크의 우선순위를 변경
	if(kMemCmp(vcID, "0x", 2) == 0){
		qwID = kAToI(vcID+2, 16);
	}
	else{
		qwID = kAToI(vcID, 10);
	}

	bPriority = kAToI(vcPriority, 10);

	kPrintf("Change Task Priority ID [0x%q] Priority[%d] ",qwID, bPriority);
	if(kChangePriority(qwID, bPriority) == TRUE)
		kPrintf("Success\n");
	else
		kPrintf("Fail\n");
}

// 현재 생성된 모든 태스크의 정보를 출력
static void kShowTaskList(const char* pcParameterBuffer){
	int i;
	TCB* pstTCB;
	int iCount = 0;

	kPrintf("========== Task Total Count [%d] ==============\n", kGetTaskCount());
	for(i=0; i<TASK_MAXCOUNT; i++){
		// TCB를 구해서 TCB가 사용 중이면 ID를 출력
		pstTCB = kGetTCBInTCBPool(i);
		if((pstTCB->stLink.qwID >>32) != 0){
			// 태스크가 10개 출력될때마다 태스크 정보를 표시할지 여부를 확인
			if((iCount != 0) && (iCount % 10) == 0){
				kPrintf("Press any key to continue... ('q' is exist) :");
				if(kGetCh() == 'q'){
					kPrintf("\n");
					break;
				}
				kPrintf("\n");
			}
			kPrintf("[%d] Task ID[0x%Q], Priority[%d], Flags[0x%Q], Thread[%d]\n",
				1 + iCount++, pstTCB->stLink.qwID, GETPRIORITY(pstTCB->qwFlags),
				pstTCB->qwFlags, kGetListCount(&(pstTCB->stChildThreadList)));
			kPrintf("    Parent PID[0x%Q], Memory Address[0x%Q], Size[0x%Q]\n", 
				pstTCB->qwParentProcessID, pstTCB->pvMemoryAddress, pstTCB->qwMemorySize);
		}
	}
}

// 태스크를 종료
static void kKillTask(const char* pcParameterBuffer){
	PARAMETERLIST stList;
	char vcID[30];
	QWORD qwID;
	TCB* pstTCB;
	int i;

	kInitializeParameter(&stList, pcParameterBuffer);
	kGetNextParameter(&stList, vcID);

	if(kMemCmp(vcID, "0x", 2) == 0)
		qwID = kAToI(vcID+2, 16);
	else
		qwID = kAToI(vcID, 10);

	if(qwID != 0xffffffff){
		pstTCB = kGetTCBInTCBPool( GETTCBOFFSET(qwID));
		qwID = pstTCB->stLink.qwID;

		// 시스텝 테스크는 제외
		if(((qwID >> 32) != 0) && ((pstTCB->qwFlags & TASK_FLAGS_SYSTEM) == 0x00)){
			kPrintf("Kill Task ID[0x%q]", qwID);
			if(kEndTask(qwID) == TRUE)
				kPrintf("Success\n");
			else
				kPrintf("Fail\n");
		}
		else {
			kPrintf("Task does not exist or task is system task\n");
		}
	}


	// 콘솔 셸과 유휴 태스크 제외 모든 태스크 종료
	else{
		for(i=2; i<TASK_MAXCOUNT; i++){
			pstTCB = kGetTCBInTCBPool(i);
			qwID = pstTCB->stLink.qwID;
			// 시스텝 테스크는 제외
			if(((qwID >> 32) != 0) && ((pstTCB->qwFlags & TASK_FLAGS_SYSTEM) == 0x00)){
				kPrintf("Kill Task ID[0x%q]", qwID);
				if(kEndTask(qwID) == TRUE)
					kPrintf("Success\n");
				else
					kPrintf("Fail\n");
			}
		}
	}

	
}

// 프로세서의 사용률을 표시
static void kCPULoad(const char* pcParameterBuffer){
	kPrintf("Processor Load : %d\n", kGetProcessorLoad());
}

static MUTEX gs_stMutex;
static volatile QWORD gs_qwAdder;

// 뮤텍스를 테스트 하는 태스크
static void kPrintNumberTask(void){
	int i;
	int j;
	QWORD qwTickCount;

	qwTickCount = kGetTickCount();
	while( (kGetTickCount() - qwTickCount) < 50)
		kSchedule();

	for(i=0; i<5; i++){
		kLock( &(gs_stMutex));
		kPrintf("Task ID [0x%Q] Value[%d]\n", kGetRunningTask()->stLink.qwID,
			gs_qwAdder);
		gs_qwAdder += 1;
		kUnlock( &(gs_stMutex));

		for(j=0; j<30000; j++);
	}

	qwTickCount = kGetTickCount();
	while( (kGetTickCount() - qwTickCount) < 1000)
		kSchedule();

	kExitTask();
}

static void kTestMutex(const char* pcParameterBuffer){
	int i;

	gs_qwAdder = 1;

	kInitializeMutex(&gs_stMutex);

	for(i=0; i<3; i++){
		kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD) kPrintNumberTask);
	}
	kPrintf("Wait Util %d Task End..\n", i);
	kGetCh();
}

// 태스크 2를 자신의 스레드로 생성하는 태스크
static void kCreateThreadTask(void){
	int i;
	for(i=0; i<3; i++){
		kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kTestTask2);
	}
	while(1){
		kSleep(1);
	}
}

// 스레드를 테스트하는 태스크 생성
static void kTestThread(const char* pcParameterBuffer){
	TCB* pstProcess;

	pstProcess = kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_PROCESS, (void*)0xeeeeeeee, 0x1000, (QWORD)kCreateThreadTask);
	if(pstProcess != NULL)
		kPrintf("Process [0x%Q] Create Success\n", pstProcess->stLink.qwID);
	else
		kPrintf("Process create Fail\n");
}

// 난수 발생시키기 위한 변수
static volatile QWORD gs_qwRandomValue = 0;

// 임의의 난수를 반환
QWORD kRandom(void){
	gs_qwRandomValue = (gs_qwRandomValue * 412153 + 5571031) >> 16;
	return gs_qwRandomValue;
}

static void kDraopCharactorThread(void){
	int iX, iY;
	int i;
	char vcText[2] = {0,};

	iX = kRandom() % CONSOLE_WIDTH;

	while(1){
		// 잠시 대기함
		kSleep(kRandom() % 20);

		if( (kRandom() %20) < 15){
			vcText[0] = ' ';
			for(i=0; i<CONSOLE_HEIGHT; i++){
				kPrintStringXY(iX, i, vcText);
				kSleep(50);
			}
		}
		else {
			for(i=0; i<CONSOLE_HEIGHT-1; i++){
				vcText[0] = kRandom() + i;
				kPrintStringXY(iX, i, vcText);
				kSleep(50);
			}
		}
	}
}

static void kMatrixProcess(void){
	int i;

	for(i=0; i<300; i++){
		if(kCreateTask(TASK_FLAGS_THREAD | TASK_FLAGS_LOW, 0, 0, (QWORD)kDraopCharactorThread) == NULL){
			break;
		}
		kSleep(kRandom()%5 +5);
	}
	kPrintf("%d Thread is created\n", i);

	kGetCh();
}

static void kShowMatrix(const char* pcParameterBuffer){
	TCB* pstProcess;

	pstProcess = kCreateTask(TASK_FLAGS_PROCESS | TASK_FLAGS_LOW, (void*)0xe00000, 0xe00000, (QWORD)kMatrixProcess);
	if(pstProcess != NULL){
		kPrintf("Matrix Process [0x%q] Create Success\n", pstProcess->pvMemoryAddress);

		// 태스크 종료가 될 때까지 대기
		while( (pstProcess->stLink.qwID >> 32) != 0)
			kSleep(100);
	}
	else
		kPrintf("Matrix Create Fail\n");
}

// FPU를 테스트하는 태스크
static void kFPUTestTask(void){
	double dValue1;
	double dValue2;
	TCB* pstRunningTask;
	QWORD qwCount = 0;
	QWORD qwRandomValue;
	int i;
	int iOffset;
	char vcData[] ={'-', '\\', '|', '/'};
	CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;

	pstRunningTask = kGetRunningTask();

	// 자신의 ID를 얻어서 화면 오프셋으로 사용
	iOffset = (pstRunningTask->stLink.qwID & 0xffffffff) * 2;
	iOffset = CONSOLE_WIDTH * CONSOLE_HEIGHT - (iOffset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));

	while(1){
		dValue1 = 1;
		dValue2 = 1;

		// 테스트를 위한 도일한 계산을 2번 반복해서 실행
		for(i=0; i<10; i++){
			qwRandomValue = kRandom();
			dValue1 *= (double)qwRandomValue;
			dValue2 *= (double)qwRandomValue;

			kSleep(1);

			qwRandomValue = kRandom();
			dValue1 /= (double)qwRandomValue;
			dValue2 /= (double)qwRandomValue;
		}
		if(dValue1 != dValue2){
			kPrintf("Value is not same! [%f] != [%f]\n", dValue1, dValue2);
			break;
		}
		qwCount++;

		// 회전하는 바람개비를 표시
		pstScreen[iOffset].bCharacter = vcData[qwCount % 4];

		// 색깔 지정
		pstScreen[iOffset].bAttribute = (iOffset % 15) + 1;
	}
}

static void kTestPIE(const char* pcParameterBuffer){
	double dResult;
	int i;

	kPrintf("PIE Cacluation Test\n");
	kPrintf("Result: 355/ 113 = ");
	dResult = (double)355/113;
	kPrintf("%d.%d%d\n", (QWORD)dResult, ((QWORD)(dResult*10)%10), ((QWORD)(dResult*100)%10));

	for(i=0; i<100; i++){
		kCreateTask(TASK_FLAGS_THREAD | TASK_FLAGS_LOW, 0, 0, (QWORD)kFPUTestTask);
	}
}

// 동적 메모리 정보를 표시
static void kShowDynamicMemoryInformation(const char* pcParameterBuffer){
	QWORD qwStartAddress, qwTotalSize, qwMetaSize, qwUsedSize;

	kGetDynamicMemoryInformation(&qwStartAddress, &qwTotalSize, &qwMetaSize, &qwUsedSize);

	kPrintf("============ Dynamic Memory Information =============\n	");
	kPrintf("Start Address : [0x%Q]\n", qwStartAddress);
	kPrintf("Total Size : [0x%Q]byte, [%d]MB\n", qwTotalSize, qwTotalSize/1024/1024);
	kPrintf("Meta Size : [0x%Q]byte, [%d]KB\n", qwMetaSize, qwMetaSize/1024);
	kPrintf("Used Size : [0x%Q]byte, [%d]KB\n", qwUsedSize, qwUsedSize/1024);
}

// 모든 블록 리스트의 블록을 순차적으로 할당하고 해체하는 테스트
static void kTestSequentialAllocation(const char* pcParameterBuffer){
	DYNAMICMEMORY* pstMemory;
	long i, j, k;
	QWORD* pqwBuffer;

	kPrintf("============ Dynamic Memory Test =============\n");
	pstMemory = kGetDynamicMemoryManager();

	for(i=0; i<pstMemory->iMaxLevelCount; i++){
		kPrintf("Block List [%d] Test Start\n", i);
		kPrintf("Allocation and Compare:");

		// 모든 블록을 할당받아서 값을 채운 후 검사
		for(j=0; j<(pstMemory->iBlockCountOfSmallestBlock >> i); j++){
			pqwBuffer = kAllocateMemory(DYNAMICMEMORY_MIN_SIZE << i);
			if(pqwBuffer == NULL){
				kPrintf("\nAllocation Fail\n");
				return ;
			}

			// 값을 채운 후 검사
			for(k=0; k<(DYNAMICMEMORY_MIN_SIZE << i)/8; k++)
				pqwBuffer[k] = k;

			for(k=0; k<(DYNAMICMEMORY_MIN_SIZE << i)/8; k++){
				if(pqwBuffer[k] != k){
					kPrintf("\nCompare Fail\n");
					return ;
				}
			}

			// 진행 과정을 . 으로 표시
			kPrintf(".");
		}

		kPrintf("\nFree:");
		for(j=0; j<(pstMemory->iBlockCountOfSmallestBlock >> i); j++){
			if(kFreeMemory( (void*)(pstMemory->qwStartAddress + (DYNAMICMEMORY_MIN_SIZE << i) * j)) == FALSE){
				kPrintf("\nFree Fail\n");
				return ;
			}
			// 진행 과정을 . 으로 표시
			kPrintf(".");
		}
		kPrintf("\n");
	}
	kPrintf("Test Complete!\n");
}

// 임의로 메모리를 할당하고 해제하는것을 반복하는 태스크
static void kRandomAllocationTask(void){
	TCB* pstTask;
	QWORD qwMemorySize;
	char vcBuffer[200];
	BYTE* pbAllocationBuffer;
	int i,j;
	int iY;

	pstTask = kGetRunningTask();
	iY = (pstTask->stLink.qwID) % 15 + 9;

	for(j=0; j<10; j++){
		// 1kb ~ 32kb까지 할당함
		do{
			qwMemorySize = ((kRandom() % (32 * 1024)) + 1) * 1024;
			pbAllocationBuffer = (BYTE*)kAllocateMemory(qwMemorySize);

			// 만일 버퍼를 할당받지 못하면 다른 태스크가 메모리를 사용하고
			// 있을수 있으므로 잠시 대기한 후 다시 시도
			if(pbAllocationBuffer == 0)
				kSleep(1);
		} while(pbAllocationBuffer == 0);

		kSPrintf(vcBuffer, "|Address: [0x%Q] Size: [0x%Q] Allocation Success", pbAllocationBuffer, qwMemorySize);
		// 자신의 ID를 Y 좌표로 하여 데이터를 출력
		kPrintStringXY(20, iY, vcBuffer);
		kSleep(200);

		// 버퍼를 반으로 나눠서 랜덤한 데이터를 똑같이 채움
		kSPrintf(vcBuffer, "|Address: [0x%Q] Size: [0x%Q] Data Write...    ", pbAllocationBuffer, qwMemorySize);
		kPrintStringXY(20, iY, vcBuffer);
		for(i=0; i<qwMemorySize/2; i++){
			pbAllocationBuffer[i] = kRandom() % 0xff;
			pbAllocationBuffer[i + (qwMemorySize /2 )] = pbAllocationBuffer[i];
		}
		kSleep(200);

		// 채운 데이터가 정상적인지 다시 확인
		kSPrintf(vcBuffer, "|Address: [0x%Q] Size: [0x%Q] Data Verify...   ", pbAllocationBuffer, qwMemorySize);
		kPrintStringXY(20, iY, vcBuffer);
		for(i=0; i<qwMemorySize/2; i++){
			if(pbAllocationBuffer[i] != pbAllocationBuffer[i + (qwMemorySize /2 )]){
				kPrintf("Task ID[0x%Q] verify Fail\n", pstTask->stLink.qwID);
				kExitTask();
			}
		}
		kFreeMemory(pbAllocationBuffer);
		kSleep(200);
	}
	kExitTask();
}

// 태스크를 여러개 생성하여 임의의 메모리를 할당하고 해제하는 것을 반복하는 테스트
static void kTestRandomAllocation(const char* pcParameterBuffer){
	int i;
	for(i=0; i<1000; i++){
		kCreateTask(TASK_FLAGS_LOWEST | TASK_FLAGS_THREAD, 0, 0, (QWORD)kRandomAllocationTask);
	}
}

// 하드 디스크의 정보를 표시
static void kShowHDDInformation(const char* pcParameterBuffer){
	HDDINFORMATION stHDD;
	char vcBuffer[100];

	// 하드 디스크의 정보를 읽음
	if( kGetHDDInformation(&stHDD)== FALSE){
		kPrintf("HDD Information Read Fail\n");
		return ;
	}

	kPrintf("============= Primary Master HDD Information============\n");

	// 모델 번호 출력
	kMemCpy(vcBuffer, stHDD.vwModelNumber, sizeof(stHDD.vwModelNumber));
	vcBuffer[sizeof(stHDD.vwModelNumber) -1 ] = '\0';
	kPrintf("Model Number:\t %s\n", vcBuffer);

	// 시리얼 번호 출력
	kMemCmp(vcBuffer, stHDD.vwSerialNumber, sizeof(stHDD.vwSerialNumber));
	kMemCpy(vcBuffer, stHDD.vwModelNumber, sizeof(stHDD.vwModelNumber));
	kPrintf("Serail Number:\t %s\n", vcBuffer);

	// 헤드, 실린더, 실린더 당 섹터 수를 출력
	kPrintf("Head Count:\t %d\n", stHDD.wNumberOfHead);
	kPrintf("Cylinder Count:\t %d\n", stHDD.wNumberOfCylinder);
	kPrintf("Sector Count:\t %d\n", stHDD.wNumberOfSectorPerCylinder);

	// 총 섹터수 출력
	kPrintf("total Sector:\t %d Sector, %dMB\n", stHDD.dwTotalSectors, stHDD.dwTotalSectors / 2 / 1024);
}

// 하드 디스크에서 파라미터로 넘어온 LBA 어드레스부터 섹터 수 만큼 읽음
static void kReadSector(const char* pcParameterBuffer){
	PARAMETERLIST stList;
	char vcLBA[50], vcSectorCount[50];
	DWORD dwLBA;
	int iSectorCount;
	char* pcBuffer;
	int i, j;
	BYTE bData;
	BOOL bExit = FALSE;

	// 파라미터 리스트를 초기화 하여 LBA 어드레스와 섹터 수 추출
	kInitializeParameter( &stList, pcParameterBuffer);
	if( (kGetNextParameter( &stList, vcLBA) == 0) || (kGetNextParameter( &stList, vcSectorCount) == 0)){
		kPrintf("ex) readsector 0(LBA) 10(count)\n");
		return ;
	}
	dwLBA = kAToI(vcLBA, 10);
	iSectorCount = kAToI(vcSectorCount, 10);

	// 섹터 수만큼 메모리를 할당 받아 읽기 수행
	pcBuffer = kAllocateMemory(iSectorCount * 512);
	if(kReadHDDSector(TRUE, TRUE, dwLBA, iSectorCount, pcBuffer) == iSectorCount){
		kPrintf("LBA[%d], [%d] Sector Read Success~!!\n", dwLBA, iSectorCount);

		// 데이터 버퍼의 내용을 출력
		for(j=0; j<iSectorCount; j++){
			for(i=0; i<512; i++){
				if( !((j==0) && (i==0)) && ((i%256) == 0)){
					kPrintf("\nPress any key to continue... ('q' is exit):");
					if(kGetCh() == 'q'){
						bExit = TRUE;
						break;
					}
				}
				if( (i%16) == 0){
					kPrintf("\n[LBA[%d, Offset:%d]\t| ", dwLBA +j, i);
				}

				// 모두 두자리로 표시하려고 16보다 작은 경우 0을 추가
				bData = pcBuffer[j*512+i] & 0xff;
				if(bData < 16){
					kPrintf("0");
				}
				kPrintf("%X ", bData);
			}
			if(bExit == TRUE)
				break;
			kPrintf("\n");
		}
	}
	else {
		kPrintf("Read Fail\n");
	}
	kFreeMemory(pcBuffer);
}

// 하드 디스크에서 파라미터로 넘어온 LBA 어드레스부터 섹터수 만큼 씀
static void kWriteSector(const char* pcParameterBuffer){
	PARAMETERLIST stList;
	char vcLBA[50], vcSectorCount[50];
	DWORD dwLBA;
	int iSectorCount;
	char* pcBuffer;
	int i, j;
	BYTE bData;
	BOOL bExit = FALSE;
	static DWORD s_dwWriteCount = 0;

	kInitializeParameter( &stList, pcParameterBuffer);
	if( (kGetNextParameter( &stList, vcLBA) == 0) || (kGetNextParameter( &stList, vcSectorCount) == 0)){
		kPrintf("ex) readsector 0(LBA) 10(count)\n");
		return ;
	}

	dwLBA = kAToI(vcLBA, 10);
	iSectorCount = kAToI(vcSectorCount, 10);

	s_dwWriteCount++;
	// 버퍼를 할당받아 데이터를 채움
	// 패턴은 4바이트의 LBA 어드레스와 4바이트의 쓰기가 수행된 횟수로 생성
	pcBuffer = kAllocateMemory(iSectorCount * 512);
	for(j=0; j<iSectorCount; j++){
		for(i=0; i<512; i+=8){
			*(DWORD*) &(pcBuffer[j*512+i]) = dwLBA +j;
			*(DWORD*) &(pcBuffer[j*512+i+4]) = s_dwWriteCount;
		}
	}

	// 쓰기 수행
	if(kWriteHDDSector(TRUE, TRUE, dwLBA, iSectorCount, pcBuffer) != iSectorCount){
		kPrintf("Write Fail\n");
		return ;
	}
	kPrintf("LBA [%d], [%d] Sector Write Success~!!", dwLBA, iSectorCount);

	// 데이터의 버퍼의 내용을 출력
	for(j=0; j<iSectorCount; j++){
		for(i=0; i<512; i++){
			if( !((j==0) && (i==0)) && ((i%256) == 0)){
				kPrintf("\nPress any key to continue... ('q' is exit):");
				if(kGetCh() == 'q'){
					bExit = TRUE;
					break;
				}
			}
			if( (i%16 == 0)){
				kPrintf("\n[LBA[%d, Offset:%d]\t| ", dwLBA+j, i);
			}

			bData = pcBuffer[j*512+i]&0xff;
			if(bData<16)
				kPrintf("0");
			kPrintf("%X ", bData);
		}
		if(bExit == TRUE)
			break;
	}
	kPrintf("\n");
	kFreeMemory(pcBuffer);
}

// 하드 디스크를 연결
static void kMountHDD(const char* pcParameterBuffer){
	if(kMount() == FALSE){
		kPrintf("HDD Mount Fail\n");
		return;
	}
	kPrintf("HDD Mount Success\n");
}

// 하드 디스크에 파일 시스템 생성(포맷)
static void kFormatHDD(const char* pcParameterBuffer){
	if(kFormat() == FALSE){
		kPrintf("HDD Format Fail\n");
		return;
	}
	kPrintf("HDD Format Success\n");
}

// 파일 시스템 정보를 표시
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
	kPrintf("===============================================================================\n");
}

static void kCreateFileInRootDirectory(const char* pcParameterBuffer){
	PARAMETERLIST stList;
	char vcFileName[50];
	int iLength;
	DWORD dwCluster; 	// 빈 클러스터 링크의 인덱스
	DIRECTORYENTRY stEntry;
	int i;

	// 파라미터 초기화
	kInitializeParameter(&stList, pcParameterBuffer);
	iLength = kGetNextParameter(&stList, vcFileName);
	vcFileName[iLength] = '\0';

	if(iLength > (sizeof(stEntry.vcFileName) -1 ) || (iLength == 0)){
		kPrintf("Wrong Usage, ex) createfile a.txt\n");
		return;
	}

	// 빈 클러스터 링크 찾아서 할단된 것으로 설정
	dwCluster = kFindFreeCluster();
	if( (dwCluster == FILESYSTEM_LASTCLUSTER) || (kSetClusterLinkData(dwCluster, FILESYSTEM_LASTCLUSTER) == FALSE)){
		kPrintf("Cluster Allocation Fail\n");
		return;
	}

	// 빈 디렉토리 엔트리 검색
	i = kFindFreeDirectoryEntry();
	if(i == -1){
		// 실패한 경우, 할당 받은 클러스터 해제
		kSetClusterLinkData(dwCluster, FILESYSTEM_FREECLUSTER);
		kPrintf("Directory is full\n");
		return;
	}

	// 디렉토리 엔트리 설정
	kMemCpy(stEntry.vcFileName, vcFileName, iLength+1);
	stEntry.dwFileSize = 0;
	stEntry.dwStartClusterIndex = dwCluster;

	// 디렉터리 엔트리 등록
	if( kSetDirectoryEntryData(i, &stEntry) == FALSE){
		kSetClusterLinkData(dwCluster, FILESYSTEM_FREECLUSTER);
		kPrintf("Directory Allocation Fail\n");
		return;
	}
	kPrintf("'%s' File Create Success\n", vcFileName);
}

static void kDeleteFileInRootDirectory(const char* pcParameterBuffer){
	PARAMETERLIST stList;
	char vcFileName[50];
	int iLength;
	DIRECTORYENTRY stEntry;
	int iOffset;

	kInitializeParameter(&stList, pcParameterBuffer);
	iLength = kGetNextParameter(&stList, vcFileName);
	vcFileName[iLength] = '\0';

	if(iLength > (sizeof(stEntry.vcFileName) -1 ) || (iLength == 0)){
		kPrintf("Wrong Usage, ex) deletefile a.txt\n");
		return;
	}

	iOffset = kFindDirectoryEntry(vcFileName, &stEntry);
	if(iOffset == -1){
		kPrintf("'%s' File not found\n", vcFileName);
		return;
	}

	if(kSetClusterLinkData(stEntry.dwStartClusterIndex, FILESYSTEM_FREECLUSTER) == FALSE){
		kPrintf("Cluster free fail\n");
		return;
	}

	kMemSet(&stEntry, 0, sizeof(DIRECTORYENTRY));
	if(kSetDirectoryEntryData(iOffset, &stEntry) == FALSE){
		kPrintf("Directory free fail\n");
		return;
	}

	kPrintf("'%s' File delete Success\n", vcFileName);
}

// 루트 디렉토리 안 모든 파일 출력
static void kShowRootDirectory(const char* pcParameterBuffer){
	BYTE* pbClusterBuffer;
	int i, iCount, iTotalCount;
	DIRECTORYENTRY* pstEntry;
	char vcBuffer[400];
	char vcTempValue[50];
	DWORD dwTotalByte;

	// 1클러스터 크기 메모리 할당
	pbClusterBuffer = (BYTE*)kAllocateMemory(FILESYSTEM_CLUSTERSIZE);

	// 루트 디렉토리(클러스터0) 읽기
	if(kReadCluster(0, pbClusterBuffer) == FALSE){
		kPrintf("Root Direcotry read fail\n");
		return;
	}

	// 루트 디렉토리 내의 총 파일 개수, 총 파일 크기를 구함
	pstEntry = (DIRECTORYENTRY*)pbClusterBuffer;
	iTotalCount = 0;
	dwTotalByte = 0;
	for(i=0; i<FILESYSTEM_MAXDIRECTORYENTRYCOUNT; i++){
		if(pstEntry[i].dwStartClusterIndex == 0x00)
			continue;

		iTotalCount++;
		dwTotalByte += pstEntry[i].dwFileSize;
	}

	pstEntry = (DIRECTORYENTRY*)pbClusterBuffer;
	iCount = 0;
	for(i = 0; i < FILESYSTEM_MAXDIRECTORYENTRYCOUNT; i++){

		// 빈 디렉토리 엔트리인 경우, 스킵
		if(pstEntry[i].dwStartClusterIndex == 0x00){
			continue;
		}

		// 출력 버퍼를 공백으로 초기화
		kMemSet(vcBuffer, ' ', sizeof(vcBuffer) - 1);
		vcBuffer[sizeof(vcBuffer)-1] = '\0';

		// 출력 버퍼에 파일 이름 설정
		kMemCpy(vcBuffer, pstEntry[i].vcFileName, kStrLen(pstEntry[i].vcFileName));

		// 출력 버퍼에 파일 크기 설정
		kSPrintf(vcTempValue, "%d byte", pstEntry[i].dwFileSize);
		kMemCpy(vcBuffer + 30, vcTempValue, kStrLen(vcTempValue));

		// 출력 버퍼에 시작 클러스터 인덱스 설정
		kSPrintf(vcTempValue, "0x%X cluster", pstEntry[i].dwStartClusterIndex);
		kMemCpy(vcBuffer + 55, vcTempValue, kStrLen(vcTempValue) + 1);

		// 파일 목록 출력
		kPrintf("    %s\n", vcBuffer);

		// 파일 목록을 20개 출력시마다 목록을 더 출력할지 여부를 확인
		if((iCount != 0) && ((iCount % 20) == 0)){

			kPrintf("Press any key to continue...('q' is exit):");

			if(kGetCh() == 'q'){
				kPrintf("\n");
				break;
			}

			kPrintf("\n");
		}

		iCount++;
	}

	// 총 파일 개수, 총 파일 크기를 출력
	kPrintf("\tTotal File Count : %d\tTotal File Size : %d byte\n", iTotalCount, dwTotalByte);

	// 메모리 해제
	kFreeMemory(pbClusterBuffer);
}