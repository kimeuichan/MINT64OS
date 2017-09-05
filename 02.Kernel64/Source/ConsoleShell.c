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

	kPrintf("===============================================================================\n");
	kPrintf("                               MINT64 Shell Help                               \n");
	kPrintf("-------------------------------------------------------------------------------\n");

	iCount = sizeof(gs_vstCommandTable) / sizeof(SHELLCOMMANDENTRY);

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

		// 도움말 목록을 15개 출력시마다 목록을 더 출력할지 여부를 확인
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
	kSetCursor(0, 1); // 첫번째 줄에는 인터럽트를 표시하기 때문에, 두번째 줄로 커서 이동
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

	// 파라미터 초기화
	kInitializeParameter(&stList, pcParameterBuffer);

	while(1){

		// 파라미터 취득 : Decimal/Hex String
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

	// 파일 시스템 캐시 버퍼의 데이터를 하드 디스크에 씀
	kPrintf("Flush File System Cache...\n");
	if(kFlushFileSystemCache() == TRUE){
		kPrintf("Success~!!\n");

	}else{
		kPrintf("Fail~!!\n");
	}

	// 키보드 컨트롤러를 통해 PC를 재부팅
	kPrintf("Press any key to reboot PC...\n");
	kGetCh();
	kReboot();
}

static void kSetTimer(const char* pcParameterBuffer){
	PARAMETERLIST stList;
	char vcParameter[100];
	long lMillisecond;
	BOOL bPeriodic;

	// 파라미터 초기화
	kInitializeParameter(&stList, pcParameterBuffer);

	// 1번째 파라미터 취득 : Millisecond
	if(kGetNextParameter(&stList, vcParameter) == 0){
		kPrintf("Wrong Usage, ex) settimer 1(ms) 1(periodic)\n");
		return;
	}

	lMillisecond = kAToI(vcParameter, 10);

	// 2번째 파라미터 취득 : Periodic
	if(kGetNextParameter(&stList, vcParameter) == 0){
		kPrintf("Wrong Usage, ex) settimer 1(ms) 1(periodic)\n");
		return;
	}

	bPeriodic = kAToI(vcParameter, 10);

	// PIT 초기화
	kInitializePIT(MSTOCOUNT(lMillisecond), bPeriodic);

	kPrintf("Time = %d ms, Periodic = %d Change Complete\n", lMillisecond, bPeriodic);
}

static void kWaitUsingPIT(const char* pcParameterBuffer){
	char vcParameter[100];
	int iLength;
	PARAMETERLIST stList;
	long lMillisecond;
	int i;

	// 파라미터 초기화
	kInitializeParameter(&stList, pcParameterBuffer);

	// 1번째 파라미터 취득 : Millisecond
	if(kGetNextParameter(&stList, vcParameter) == 0){
		kPrintf("Wrong Usage, ex) wait 1(ms)\n");
		return;
	}

	lMillisecond = kAToI(vcParameter, 10);

	kPrintf("%d ms Sleep Start...\n", lMillisecond);

	// 인터럽트를 비활성화하고, PIT 컨트롤러를 통해 직접 시간을 측정
	kDisableInterrupt();
	for(i = 0; i < (lMillisecond / 30); i++){
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

	kPrintf("Now Measuring");

	// 10초 동안 변화한 Time Stamp Counter를 이용하여 프로세서의 속도를 간접적으로 측정
	kDisableInterrupt();
	for(i = 0; i < 200; i++){
		qwLastTSC = kReadTSC();
		kWaitUsingDirectPIT(MSTOCOUNT(50));
		qwTotalTSC += (kReadTSC() - qwLastTSC);
		kPrintf(".");
	}
	// 타이머 복원
	kInitializePIT(MSTOCOUNT(1), TRUE);
	kEnableInterrupt();

	kPrintf("\nCPU Speed = %d MHz\n", qwTotalTSC / 10 / 1000 / 1000);
}

static void kShowDateAndTime(const char* pcParameterBuffer){
	WORD wYear;
	BYTE bMonth, bDayOfMonth, bDayOfWeek;
	BYTE bHour, bMinute, bSecond;

	// RTC 컨트롤러에서 현재 시간 및 날짜를 읽음
	kReadRTCTime(&bHour, &bMinute, &bSecond);
	kReadRTCDate(&wYear, &bMonth, &bDayOfMonth, &bDayOfWeek);

	kPrintf("Date: %d-%d-%d %d:%d:%d (%s)\n", wYear, bMonth, bDayOfMonth, bHour, bMinute, bSecond, kConvertDayOfWeekToString(bDayOfWeek));
}

// 테스트 태스크 1 : 화면 테두리를 돌면서 문자를 출력
static void kTestTask1(void){
	BYTE bData;
	int i = 0, iX = 0, iY = 0, iMargin, j;
	CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
	TCB* pstRunningTask;

	// TCB ID의 일련 번호를 화면 오프셋으로 이용
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

		// 라운드 로빈 스케줄러->멀티레벨 큐 스케줄러로 업그레이드했기 때문에, 태스크 전환은 주석 처리
		//kSchedule();
	}

	// 스택에 복귀 주소를 삽입했기 때문에, 태스크 종료는 주석 처리
	//kExitTask();
}

// 테스트 태스크 2 : 자신의 TCB ID의 일련 번호를 참고하여 특정 위치에 회전하는 바람개비를 출력
static void kTestTask2(void){
	int i = 0, iOffset;
	CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
	TCB* pstRunningTask;
	char vcData[4] = {'-', '\\', '|', '/'};

	// 현재 태스크 ID의 오프셋을 화면 오프셋으로 사용
	pstRunningTask = kGetRunningTask();
	iOffset = (pstRunningTask->stLink.qwID & 0xFFFFFFFF) * 2;
	iOffset = (CONSOLE_WIDTH * CONSOLE_HEIGHT) - (iOffset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));

	// 무한 루프
	while(1){

		// 회전하는 바람개비 출력
		pstScreen[iOffset].bCharacter = vcData[i % 4];
		pstScreen[iOffset].bAttribute = (iOffset % 15) + 1;
		i++;

		// 라운드 로빈 스케줄러->멀티레벨 큐 스케줄러로 업그레이드했기 때문에, 태스크 전환은 주석 처리
		//kSchedule();
	}
}

static void kCreateTestTask(const char* pcParameterBuffer){
	PARAMETERLIST stList;
	char vcParameter[100];
	long lType, lCount;
	int i;

	// 파라미터 초기화
	kInitializeParameter(&stList, pcParameterBuffer);

	// 1번째 파라미터 취득 : Type
	if(kGetNextParameter(&stList, vcParameter) == 0){
		kPrintf("Wrong Usage, ex) createtask 1(type) 1022(count)\n");
		return;
	}

	lType = kAToI(vcParameter, 10);

	// 2번째 파라미터 취득 : Count
	if(kGetNextParameter(&stList, vcParameter) == 0){
		kPrintf("Wrong Usage, ex) createtask 1(type) 1022(count)\n");
		return;
	}

	lCount = kAToI(vcParameter, 10);

	switch(lType){
	case 1: // 테스트 태스크 1 : 테두리 문자 출력
		for(i = 0; i < lCount; i++){
			if(kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_THREAD, 0, 0, (QWORD)kTestTask1) == NULL){
				break;
			}
		}

		kPrintf("kTestTask1 Created(%d)\n", i);
		break;

	case 2: // 테스트 태스크 2 : 바람개비 출력
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

	// 파라미터 초기화
	kInitializeParameter(&stList, pcParameterBuffer);

	// 1번째 파라미터 취득 : TaskID
	if(kGetNextParameter(&stList, vcTaskID) == 0){
		kPrintf("Wrong Usage, ex) changepriority 0x300000002(taskId) 0(priority)\n");
		return;
	}

	// 2번째 파라미터 취득 : Priority
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

	// Task Total Count 라인 채우기
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

		// 태스크 ID의 상위 32비트(TCB 할당 횟수)가 0이 아닌지 확인
		if((pstTCB->stLink.qwID >> 32) != 0){

			// 태스크 정보를 5개 출력시마다 정보를 더 출력할지 여부를 확인
			if((iCount != 0) && ((iCount % 5) == 0)){

				kPrintf("Press any key to continue...('q' is exit):");

				if(kGetCh() == 'q'){
					kPrintf("\n");
					break;
				}

				kPrintf("\n");
			}

			// 태스크 리스트 출력 정보
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

	// 파라미터 초기화
	kInitializeParameter(&stList, pcParameterBuffer);

	// 1번째 파라미터 취득 : TaskID
	if(kGetNextParameter(&stList, vcTaskID) == 0){
		kPrintf("Wrong Usage, ex) killtask 0x300000002(taskId) or all\n");
		return;
	}

	// 특정 ID의 태스크만 종료
	if(kMemCmp(vcTaskID, "all", 3) != 0){

		if(kMemCmp(vcTaskID, "0x", 2) == 0){
			qwTaskID = kAToI(vcTaskID + 2, 16);

		}else{
			qwTaskID = kAToI(vcTaskID, 10);
		}

		// [주의]파라미터 TaskID를 태스크 풀의 실제 TaskID로 덮어 썼으므로, 파라미터 TaskID의 오프셋(하위 32비트)정보만 제대로 입력하면 해당 태스크가 종료됨
		pstTCB = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
		qwTaskID = pstTCB->stLink.qwID;

		// 할당(생성)되지 않은 태스크와 시스템 태스크는 종료 대상에서 제외
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

	// 모든 태스크 종료(단, 콘솔 쉘 태스크, 유휴 태스크는 제외)
	}else{
		for(i = 0; i < TASK_MAXCOUNT; i++){
			pstTCB = kGetTCBInTCBPool(i);
			qwTaskID = pstTCB->stLink.qwID;

			// 할당(생성)되지 않은 태스크와 시스템 태스크는 종료 대상에서 제외
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

/***** 전역 변수 선언 *****/
// 뮤텍스 테스트용 뮤텍스와 변수
static MUTEX gs_stMutex;
static volatile QWORD gs_qwAdder;

static void kPrintNumberTask(const char* pcParameterBuffer){
	int i, j;
	QWORD qwTickCount;

	// 50ms 정도 대기하여, 뮤텍스 테스트용 출력 메세지와 콘솔 쉘의 출력 메세지가 겹치지 않도록 함
	qwTickCount = kGetTickCount();
	while((kGetTickCount() - qwTickCount) < 50){
		kSchedule();
	}

	// 뮤텍스 테스트용 숫자 출력
	for(i = 0; i < 5; i++){
		kLock(&(gs_stMutex));

		kPrintf("Test Mutex : TaskID=[0x%Q] Value=[%d]\n", kGetRunningTask()->stLink.qwID, gs_qwAdder);
		gs_qwAdder++;

		kUnlock(&(gs_stMutex));

		// 프로세서의 소모를 늘리려고 추가한 코드
		for(j = 0; j < 30000; j++);
	}

	// 모든 태스크가 숫자 출력을 완료할 때까지 1000ms 정도 대기하여, 뮤텍스 테스트용 출력 메세지와 유휴 태스크의 태스크 종료 메세지가 겹치지 않도록 함
	qwTickCount = kGetTickCount();
	while((kGetTickCount() - qwTickCount) < 1000){
		kSchedule();
	}

	// 스택에 복귀 주소를 삽입했기 때문에, 태스크 종료는 주석 처리
	// kExitTask();
}

static void kTestMutex(const char* pcParameterBuffer){
	int i;

	gs_qwAdder = 1;

	// 뮤텍스 초기화
	kInitializeMutex(&gs_stMutex);

	// 뮤텍스 테스트용 태스크 3개 생성
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

	// 프로세스 1개와 스레드 3개를 생성
	pstProcess = kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_PROCESS, (void*)0xEEEEEEEE, 0x1000, (QWORD)kCreateThreadTask);

	if(pstProcess != NULL){
		kPrintf("Process [0x%Q] Create Success\n", pstProcess->stLink.qwID);

	}else{
		kPrintf("Process Create Fail\n");
	}
}

/***** 전역 변수 정의 *****/
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

	// 키가 입력되면 프로세스 종료
	kGetCh();
}

static void kShowMatrix(const char* pcParameterBuffer){
	TCB* pstProcess;

	pstProcess = kCreateTask(TASK_FLAGS_LOW | TASK_FLAGS_PROCESS, (void*)0xE00000, 0xE00000, (QWORD)kMatrixProcess);

	if(pstProcess != NULL){
		kPrintf("Matrix Process [0x%Q] Create Success\n", pstProcess->stLink.qwID);

		// 프로세스가 종료될 때까지 대기
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

	// 현재 태스크 ID의 오프셋을 화면 오프셋으로 사용
	pstRunningTask = kGetRunningTask();
	iOffset = (pstRunningTask->stLink.qwID & 0xFFFFFFFF) * 2;
	iOffset = (CONSOLE_WIDTH * CONSOLE_HEIGHT) - (iOffset % (CONSOLE_WIDTH * CONSOLE_HEIGHT));

	// 무한 루프
	while(1){
		dValue1 = 1;
		dValue2 = 1;

		// 테스트를 위해 동일한 계산을 2번 반복해서 실행
		for(i = 0; i < 10; i++){
			qwRandomValue = kRandom();
			dValue1 *= (double)qwRandomValue;
			dValue2 *= (double)qwRandomValue;

			kSleep(1);

			qwRandomValue = kRandom();
			dValue1 /= (double)qwRandomValue;
			dValue2 /= (double)qwRandomValue;

		}

		// FPU 연산에 문제가 발생한 경우, 에러 메세지를 출력하고 태스크 종료
		if(dValue1 != dValue2){
			kPrintf("Value is not same~!!, [%f] != [%f]\n", dValue1, dValue2);
			break;
		}

		// FPU 연산에 문제가 발생하지 않은 경우, 회전하는 바람개비를 출력
		pstScreen[iOffset].bCharacter = vcData[qwCount % 4];
		pstScreen[iOffset].bAttribute = (iOffset % 15) + 1;
		qwCount++;
	}
}

static void kTestPIE(const char* pcParameterBuffer){
	double dResult;
	int i;

	// 파이 계산 후, 출력
	kPrintf("PIE Calculation Test\n");
	kPrintf("PIE : 355 / 113 = ");
	dResult = (double)355 / 113;
	//kPrintf("%d.%d%d\n", (QWORD)dResult, ((QWORD)(dResult * 10) % 10), ((QWORD)(dResult * 100) % 10));
	kPrintf("%f\n", dResult);

	// 실수를 계산하는 태스크(회전하는 바람개비)를 100개 생성
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

		// 모든 크기의 블록을 할당받아서 값을 채운 후 검사
		kPrintf("Allocation and Compare: ");

		for(j = 0; j < (pstMemory->iBlockCountOfSmallestBlock >> i); j++){

			pqwBuffer = (QWORD*)kAllocateMemory(DYNAMICMEMORY_MIN_SIZE << i);
			if(pqwBuffer == NULL){
				kPrintf("\nAllocation Fail~!!\n");
				return;
			}

			// 할당 받은 메모리에 값을 채움
			for(k = 0; k < ((DYNAMICMEMORY_MIN_SIZE << i) / 8); k++){
				pqwBuffer[k] = k;
			}

			// 검사
			for(k = 0; k < ((DYNAMICMEMORY_MIN_SIZE << i) / 8); k++){
				if(pqwBuffer[k] != k){
					kPrintf("\nCompare Fail~!!\n");
					return;
				}
			}

			// 진행과정을 .으로 표시
			kPrintf(".");
		}

		// 할당받은 블록을 모두 해제
		kPrintf("\nFree: ");
		for(j = 0; j < (pstMemory->iBlockCountOfSmallestBlock >> i); j++){
			if(kFreeMemory((void*)(pstMemory->qwStartAddress + ((DYNAMICMEMORY_MIN_SIZE << i) * j))) == FALSE){
				kPrintf("\nFree Fail~!!\n");
				return;
			}

			// 진행과정을 .으로 표시
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
		// 1KB~32MB 크기의 메모리를 할당받음
		do{
			qwMemorySize = ((kRandom() % (32 * 1024)) + 1) * 1024;
			pbAllocationBuffer = (BYTE*)kAllocateMemory(qwMemorySize);

			// 메모리를 할당받지 못했을 경우, 다른 태스크가 메모리를 사용하고 있을 수 있으므로 잠시 대기한 후 다시 시도
			if(pbAllocationBuffer == 0){
				kSleep(1);
			}

		}while(pbAllocationBuffer == 0);

		kSPrintf(vcBuffer, "|Address=[0x%Q], Size=[0x%Q] Allocation Success~!!", pbAllocationBuffer, qwMemorySize);
		kPrintStringXY(20, iY, vcBuffer);
		kSleep(200);

		// 버퍼를 반으로 나눠서 랜덤한 데이터를 똑같이 채움
		kSPrintf(vcBuffer, "|Address=[0x%Q], Size=[0x%Q] Data Write...", pbAllocationBuffer, qwMemorySize);
		kPrintStringXY(20, iY, vcBuffer);

		for(i = 0; i < (qwMemorySize / 2); i++){
			pbAllocationBuffer[i] = kRandom() & 0xFF;
			pbAllocationBuffer[i+(qwMemorySize/2)] = pbAllocationBuffer[i];
		}

		kSleep(200);

		// 채운 데이터가 정상적인지 확인
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

	// 하드 디스크 정보를 읽음
	if(kGetHDDInformation(&stHDD) == FALSE){
		kPrintf("HDD Information Read Fail~!!");
		return;
	}

	kPrintf("======================= Primary Master HDD Information ========================\n");

	// 모델 번호 출력
	kMemCpy(vcBuffer, stHDD.vwModelNumber, sizeof(stHDD.vwModelNumber));
	vcBuffer[sizeof(stHDD.vwModelNumber) - 1] = '\0';
	kPrintf("Model Number   : %s\n", vcBuffer);

	// 시리얼 번호 출력
	kMemCpy(vcBuffer, stHDD.vwSerialNumber, sizeof(stHDD.vwSerialNumber));
	vcBuffer[sizeof(stHDD.vwSerialNumber) - 1] = '\0';
	kPrintf("Serial Number  : %s\n", vcBuffer);

	// 실린더 수, 헤드 수, 실린더당 섹터 수 출력
	kPrintf("Cylinder Count : %d\n", stHDD.wNumberOfCylinder);
	kPrintf("Head Count     : %d\n", stHDD.wNumberOfHead);
	kPrintf("Sector Count   : %d\n", stHDD.wNumberOfSectorPerCylinder);

	// 총 섹터 수 출력
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

	// 파라미터 초기화
	kInitializeParameter(&stList, pcParameterBuffer);

	// 1번째 파라미터 취득 : LBA
	if(kGetNextParameter(&stList, vcLBA) == 0){
		kPrintf("Wrong Usage, ex) readsector 0(LBA) 10(count)\n");
		return;
	}

	// 2번째 파라미터 취득 : SectorCount
	if(kGetNextParameter(&stList, vcSectorCount) == 0){
		kPrintf("Wrong Usage, ex) readsector 0(LBA) 10(count)\n");
		return;
	}

	dwLBA = kAToI(vcLBA, 10);
	iSectorCount = kAToI(vcSectorCount, 10);

	// 섹터 수만큼의 메모리를 할당
	pcBuffer = (char*)kAllocateMemory(iSectorCount * 512);

	// 섹터 읽기 수행
	if(kReadHDDSector(TRUE, TRUE, dwLBA, iSectorCount, pcBuffer) == iSectorCount){
		kPrintf("HDD Sector Read : LBA=[%d], Count=[%d] : Success~!!", dwLBA, iSectorCount);

		// 메모리 버퍼의 내용 출력
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

				// 모두 두 자리로 표시하기 위해, 16보다 작은 경우 0을 추가
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

	// 파라미터 초기화
	kInitializeParameter(&stList, pcParameterBuffer);

	// 1번째 파라미터 취득 : LBA
	if(kGetNextParameter(&stList, vcLBA) == 0){
		kPrintf("Wrong Usage, ex) writesector 0(LBA) 10(count)\n");
		return;
	}

	// 2번째 파라미터 취득 : SectorCount
	if(kGetNextParameter(&stList, vcSectorCount) == 0){
		kPrintf("Wrong Usage, ex) writesector 0(LBA) 10(count)\n");
		return;
	}

	dwLBA = kAToI(vcLBA, 10);
	iSectorCount = kAToI(vcSectorCount, 10);

	s_dwWriteCount++;

	// 섹터 수만큼의 메모리를 할당받은 후, 데이터를 채움(데이터 패턴은 LBA 어드레스(4 byte)와 쓰기 수행 횟수(4 byte)로 생성)
	pcBuffer = (char*)kAllocateMemory(iSectorCount * 512);
	for(j = 0; j < iSectorCount; j++){
		for(i = 0; i < 512; i += 8){
			*(DWORD*)&(pcBuffer[j*512+i]) = dwLBA + j;
			*(DWORD*)&(pcBuffer[j*512+i+4]) = s_dwWriteCount;
		}
	}

	// 섹터 쓰기 수행
	if(kWriteHDDSector(TRUE, TRUE, dwLBA, iSectorCount, pcBuffer) != iSectorCount){
		kPrintf("HDD Sector Write : Fail~!!\n");
	}

	kPrintf("HDD Sector Write : LBA=[%d], Count=[%d] : Success~!!", dwLBA, iSectorCount);

	// 메모리 버퍼의 내용 출력
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

			// 모두 두 자리로 표시하기 위해, 16보다 작은 경우 0을 추가
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

	// 파라미터 초기화
	kInitializeParameter(&stList, pcParameterBuffer);

	// 1번째 파라미터 취득 : FileName
	if((iLength = kGetNextParameter(&stList, vcFileName)) == 0){
		kPrintf("Wrong Usage, ex) createfile a.txt\n");
		return;
	}

	vcFileName[iLength] = '\0';

	if(iLength > (FILESYSTEM_MAXFILENAMELENGTH - 1)){
		kPrintf("Wrong Usage, Too Long File Name\n");
		return;
	}

	// 파일 열기
	pstFile = fopen(vcFileName, "w");
	if(pstFile == NULL){
		kPrintf("'%s' File Create Fail~!!\n", vcFileName);
		return;
	}

	// 파일 닫기
	fclose(pstFile);
}

static void kDeleteFileInRootDirectory(const char* pcParameterBuffer){
	PARAMETERLIST stList;
	char vcFileName[50];
	int iLength;

	// 파라미터 초기화
	kInitializeParameter(&stList, pcParameterBuffer);

	// 1번째 파라미터 취득 : FileName
	if((iLength = kGetNextParameter(&stList, vcFileName)) == 0){
		kPrintf("Wrong Usage, ex) deletefile a.txt\n");
		return;
	}

	vcFileName[iLength] = '\0';

	if(iLength > (FILESYSTEM_MAXFILENAMELENGTH - 1)){
		kPrintf("Wrong Usage, Too Long File Name\n");
		return;
	}

	// 파일 삭제
	if(remove(vcFileName) != 0){
		kPrintf("'%s' File Delete Fail~!!\n", vcFileName);
		return;
	}
}

static void kShowRootDirectory(const char* pcParameterBuffer){
	DIR* pstDirectory;
	int i, iCount, iTotalCount;
	struct dirent* pstEntry;
	char vcBuffer[76]; // 버퍼 크기(76 byte)를 텍스트 비디오 메모리(화면)의 한 라인 길이에 맞춤
	char vcTempValue[50];
	DWORD dwTotalByte;
	DWORD dwUsedClusterCount;
	FILESYSTEMMANAGER stManager;

	// 파일 시스템 정보 취득
	kGetFileSystemInformation(&stManager);

	// 루트 디렉토리 열기(루트 디렉토리밖에 없으므로, 디렉토리 이름["/"]은 무시)
	pstDirectory = opendir("/");
	if(pstDirectory == NULL){
		kPrintf("Root Directory Open Fail~!!\n");
		return;
	}

	// 루트 디렉토리내의 총 파일 개수, 총 파일 크기, 사용된 클러스터 개수를 구함
	iTotalCount = 0;
	dwTotalByte = 0;
	dwUsedClusterCount = 0;
	while(1){
		// 루트 디렉토리 읽기
		pstEntry = readdir(pstDirectory);
		if(pstEntry == NULL){
			break;
		}

		// 총 파일 개수, 총 파일 크기 계산
		iTotalCount++;
		dwTotalByte += pstEntry->dwFileSize;

		// 사용된 클러스터 개수 계산
		if(pstEntry->dwFileSize == 0){
			// 파일 크기가 0인 경우라도 클러스터 1개는 할당되어 있음
			dwUsedClusterCount++;

		}else{
			// 파일 크기를 클러스터의 크기 단위로 정렬(올림 처리)하여, 사용된 클러스터 개수를 구함
			dwUsedClusterCount += ((pstEntry->dwFileSize + (FILESYSTEM_CLUSTERSIZE - 1)) / FILESYSTEM_CLUSTERSIZE);
		}
	}

	// 파일 목록 출력 루프 처리
	rewinddir(pstDirectory);
	iCount = 0;
	while(1){
		// 루트 디렉토리 읽기
		pstEntry = readdir(pstDirectory);
		if(pstEntry == NULL){
			break;
		}

		// 출력 버퍼를 공백으로 초기화
		kMemSet(vcBuffer, ' ', sizeof(vcBuffer) - 1);
		vcBuffer[sizeof(vcBuffer)-1] = '\0';

		// 출력 버퍼에 파일 이름 설정
		kMemCpy(vcBuffer, pstEntry->d_name, kStrLen(pstEntry->d_name));

		// 출력 버퍼에 파일 크기 설정
		kSPrintf(vcTempValue, "%d byte", pstEntry->dwFileSize);
		kMemCpy(vcBuffer + 30, vcTempValue, kStrLen(vcTempValue));

		// 출력 버퍼에 시작 클러스터 인덱스 설정
		kSPrintf(vcTempValue, "0x%X cluster", pstEntry->dwStartClusterIndex);
		kMemCpy(vcBuffer + 55, vcTempValue, kStrLen(vcTempValue));

		// 파일 목록 출력
		kPrintf("    %s\n", vcBuffer);

		// 파일 목록을 15개 출력시마다 목록을 더 출력할지 여부를 확인
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

	// 총 파일 개수, 총 파일 크기, 하드 디스크의 여유 공간(남은 공간)를 출력
	kPrintf("\t\tTotal File Count : %d\n", iTotalCount);
	kPrintf("\t\tTotal File Size  : %d byte (%d cluster)\n", dwTotalByte, dwUsedClusterCount);
	kPrintf("\t\tFree Space       : %d KB (%d cluster)\n", (stManager.dwTotalClusterCount - dwUsedClusterCount) * FILESYSTEM_CLUSTERSIZE / 1024, stManager.dwTotalClusterCount - dwUsedClusterCount);

	// 루트 디렉토리 닫기
	closedir(pstDirectory);
}

static void kWriteDataToFile(const char* pcParameterBuffer){
	PARAMETERLIST stList;
	char vcFileName[50];
	int iLength;
	FILE* fp;
	int iEnterCount;
	BYTE bKey;

	// 파라미터 초기화
	kInitializeParameter(&stList, pcParameterBuffer);

	// 1번째 파라미터 취득 : FileName
	if((iLength = kGetNextParameter(&stList, vcFileName)) == 0){
		kPrintf("Wrong Usage, ex) writefile a.txt\n");
		return;
	}

	vcFileName[iLength] = '\0';

	if(iLength > (FILESYSTEM_MAXFILENAMELENGTH - 1)){
		kPrintf("Wrong Usage, Too Long File Name\n");
		return;
	}

	// 파일 열기
	fp = fopen(vcFileName, "w");
	if(fp == NULL){
		kPrintf("'%s' File Open Fail~!!\n", vcFileName);
		return;
	}

	// 파일 쓰기 루프
	iEnterCount = 0;
	while(1){
		bKey = kGetCh();

		// 엔터 키를 3번 연속 눌렀을 경우, 종료
		if(bKey == KEY_ENTER){
			iEnterCount++;
			if(iEnterCount >= 3){
				break;
			}
		}else{
			iEnterCount = 0;
		}

		kPrintf("%c", bKey);

		// 파일 쓰기
		if(fwrite(&bKey, 1, 1, fp) != 1){
			kPrintf("'%s' File Write Fail~!!\n", vcFileName);
			break;
		}
	}

	// 파일 닫기
	fclose(fp);
}

static void kReadDataFromFile(const char* pcParameterBuffer){
	PARAMETERLIST stList;
	char vcFileName[50];
	int iLength;
	FILE* fp;
	int iEnterCount;
	BYTE bKey;

	// 파라미터 초기화
	kInitializeParameter(&stList, pcParameterBuffer);

	// 1번째 파라미터 취득 : FileName
	if((iLength = kGetNextParameter(&stList, vcFileName)) == 0){
		kPrintf("Wrong Usage, ex) readfile a.txt\n");
		return;
	}

	vcFileName[iLength] = '\0';

	if(iLength > (FILESYSTEM_MAXFILENAMELENGTH - 1)){
		kPrintf("Wrong Usage, Too Long File Name\n");
		return;
	}

	// 파일 열기
	fp = fopen(vcFileName, "r");
	if(fp == NULL){
		kPrintf("'%s' File Open Fail~!!\n", vcFileName);
		return;
	}

	// 파일 읽기 루프
	iEnterCount = 0;
	while(1){
		// 파일 읽기
		if(fread(&bKey, 1, 1, fp) != 1){
			break;
		}

		kPrintf("%c", bKey);

		// 엔터 키인 경우, 횟수 증가
		if(bKey == KEY_ENTER){
			iEnterCount++;

			// 파일 내용을 15라인 출력시마다 내용을 더 출력할지 여부를 확인
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

	// 파일 닫기
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

	// 4MB 메모리(버퍼) 할당
	dwMaxFileSize = 4 * 1024 * 1024;
	pbBuffer = (BYTE*)kAllocateMemory(dwMaxFileSize);
	if(pbBuffer == NULL){
		kPrintf("Memory Allocation Fail~!!\n");
		return;
	}

	// 테스트용 파일 삭제
	remove("testfileio.bin");

	//----------------------------------------------------------------------------------------------------
	// 1. 파일 열기 실패 테스트
	//----------------------------------------------------------------------------------------------------
	kPrintf("1. File Open Fail Test...");

	// 읽기 모드(r)는 파일이 존재하지 않는 경우 파일을 생성하지 않으므로, NULL을 리턴
	pstFile = fopen("testfileio.bin", "r");
	if(pstFile == NULL){
		kPrintf("[Pass]\n");

	}else{
		kPrintf("[Fail]\n");
		fclose(pstFile);
	}

	//----------------------------------------------------------------------------------------------------
	// 2. 파일 생성 테스트
	//----------------------------------------------------------------------------------------------------
	kPrintf("2. File Create Test...");

	// 쓰기 모드(w)는 파일이 존재하지 않는 경우 파일을 생성하므로, 파일 핸들을 리턴
	pstFile = fopen("testfileio.bin", "w");
	if(pstFile != NULL){
		kPrintf("[Pass]\n");
		kPrintf("    FileHandle=[0x%Q]\n", pstFile);

	}else{
		kPrintf("[Fail]\n");
	}

	//----------------------------------------------------------------------------------------------------
	// 3. 순차적인 영역 쓰기 테스트
	//----------------------------------------------------------------------------------------------------
	kPrintf("3. Sequential Write Test(Cluster Size)...");

	// 열린 파일에 쓰기 수행
	for(i = 0; i < 100; i++){

		kMemSet(pbBuffer, i, FILESYSTEM_CLUSTERSIZE);

		// 파일 쓰기
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
	// 4. 순차적인 영역 읽기 및 검증 테스트
	//----------------------------------------------------------------------------------------------------
	kPrintf("4. Sequential Read and Verify Test(Cluster Size)...");

	// 파일의 처음으로 이동
	fseek(pstFile, -100 * FILESYSTEM_CLUSTERSIZE, SEEK_END);

	// 열린 파일에서 읽기 수행 후 데이터 검증
	for(i = 0; i < 100; i++){

		// 파일 읽기
		if(fread(pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile) != FILESYSTEM_CLUSTERSIZE){
			kPrintf("[Fail]\n");
			break;
		}

		// 데이터 검증
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
	// 5. 임의의 영역 쓰기 테스트
	//----------------------------------------------------------------------------------------------------
	kPrintf("5. Random Write Test\n");

	// 버퍼를 0으로 채움
	kMemSet(pbBuffer, 0, dwMaxFileSize);

	// 파일의 처음으로 이동 후, 파일의 내용을 읽어서 버퍼로 복사
	fseek(pstFile, -100 * FILESYSTEM_CLUSTERSIZE, SEEK_CUR);
	fread(pbBuffer, 1, dwMaxFileSize, pstFile);

	// 파일과 버퍼의 임의의 위치에 동일한 데이터를 씀
	for(i = 0; i < 100; i++){
		dwByteCount = (kRandom() % (sizeof(vbTempBuffer) - 1)) + 1;
		dwRandomOffset = kRandom() % (dwMaxFileSize - dwByteCount);
		kPrintf("    [%d] Offset=[%d], Byte=[%d]...", i, dwRandomOffset, dwByteCount);

		// 파일의 임의의 위치에 쓰기
		fseek(pstFile, dwRandomOffset, SEEK_SET);
		kMemSet(vbTempBuffer, i, dwByteCount);
		if(fwrite(vbTempBuffer, 1, dwByteCount, pstFile) != dwByteCount){
			kPrintf("[Fail]\n");
			break;

		}else{
			kPrintf("[Pass]\n");
		}

		// 버퍼의 임의의 위치에 쓰기
		kMemSet(pbBuffer + dwRandomOffset, i, dwByteCount);
	}

	// 파일과 버퍼의 마지막으로 이동하여 1바이트를 써서, 크기를 4MB로 만듦
	fseek(pstFile, dwMaxFileSize - 1, SEEK_SET);
	fwrite(&i, 1, 1, pstFile);
	pbBuffer[dwMaxFileSize - 1] = (BYTE)i;

	//----------------------------------------------------------------------------------------------------
	// 6. 임의의 영역 읽기 및 검증 테스트
	//----------------------------------------------------------------------------------------------------
	kPrintf("6. Random Read and Verify Test\n");

	// 파일과 버퍼의 임의의 위치에서 데이터를 읽은 후, 데이터 검증
	for(i = 0; i < 100; i++){
		dwByteCount = (kRandom() % (sizeof(vbTempBuffer) - 1)) + 1;
		dwRandomOffset = kRandom() % (dwMaxFileSize - dwByteCount);
		kPrintf("    [%d] Offset=[%d], Byte=[%d]...", i, dwRandomOffset, dwByteCount);

		// 파일의 임의의 위치에서 읽기
		fseek(pstFile, dwRandomOffset, SEEK_SET);
		if(fread(vbTempBuffer, 1, dwByteCount, pstFile) != dwByteCount){
			kPrintf("[Fail]\n");
			kPrintf("    File Read Fail\n");
			break;
		}

		// 데이터 검증
		if(kMemCmp(pbBuffer + dwRandomOffset, vbTempBuffer, dwByteCount) != 0){
			kPrintf("[Fail]\n");
			kPrintf("    Data Verify Fail\n");
			break;
		}

		kPrintf("[Pass]\n");
	}

	//----------------------------------------------------------------------------------------------------
	// 7. 순차적인 영역 쓰기, 읽기 및 검증 테스트
	//----------------------------------------------------------------------------------------------------
	kPrintf("7. Sequential Write, Read and Verify Test(1024 byte)\n");

	// 파일의 처음으로 이동
	fseek(pstFile, -dwMaxFileSize, SEEK_CUR);

	// 파일 쓰기 루프(앞부분에 2MB만 씀)
	for(i = 0; i < (2 * 1024 * 1024 / 1024); i++){
		kPrintf("    [%d] Offset=[%d], Byte=[%d] Write...", i, i * 1024, 1024);

		// 파일 쓰기(1024 byte씩 씀)
		if(fwrite(pbBuffer + (i * 1024), 1, 1024, pstFile) != 1024){
			kPrintf("[Fail]\n");
			break;

		}else{
			kPrintf("[Pass]\n");
		}
	}

	// 파일의 처음으로 이동
	fseek(pstFile, -dwMaxFileSize, SEEK_SET);

	// 파일 읽기 후, 데이터 검증(Random Write로 데이터가 잘못 저장되었을 수도 있으므로, 검증은 4MB 전체 영역을 대상으로 함)
	for(i = 0; i < (dwMaxFileSize / 1024); i++){
		kPrintf("    [%d] Offset=[%d], Byte=[%d] Read and Verify...", i, i * 1024, 1024);

		// 파일 읽기(1024 byte씩 읽음)
		if(fread(vbTempBuffer, 1, 1024, pstFile) != 1024){
			kPrintf("[Fail]\n");
			break;
		}

		// 데이터 검증
		if(kMemCmp(pbBuffer + (i * 1024), vbTempBuffer, 1024) != 0){
			kPrintf("[Fail]\n");
			break;

		}else{
			kPrintf("[Pass]\n");
		}
	}

	//----------------------------------------------------------------------------------------------------
	// 8. 파일 삭제 실패 테스트
	//----------------------------------------------------------------------------------------------------
	kPrintf("8. File Delete Fail Test...");

	// 현재 파일이 열려 있는 상태이므로, 파일 삭제에 실패해야 함
	if(remove("testfileio.bin") != 0){
		kPrintf("[Pass]\n");

	}else{
		kPrintf("[Fail]\n");
	}

	//----------------------------------------------------------------------------------------------------
	// 9. 파일 닫기 테스트
	//----------------------------------------------------------------------------------------------------
	kPrintf("9. File Close Fail Test...");

	// 파일 닫기
	if(fclose(pstFile) == 0){
		kPrintf("[Pass]\n");

	}else{
		kPrintf("[Fail]\n");
	}

	//----------------------------------------------------------------------------------------------------
	// 10. 파일 삭제 테스트
	//----------------------------------------------------------------------------------------------------
	kPrintf("10. File Delete Test...");

	// 파일 삭제
	if(remove("testfileio.bin") == 0){
		kPrintf("[Pass]\n");

	}else{
		kPrintf("[Fail]\n");
	}

	// 메모리(버퍼) 해제
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

	// 파일 크기 설정(클러스터 단위:1MB, 바이트 단위:16KB)
	dwClusterTestFileSize = 1024 * 1024;
	dwOneByteTestFileSize = 16 * 1024;

	// 메모리 할당
	pbBuffer = (BYTE*)kAllocateMemory(dwClusterTestFileSize);
	if(pbBuffer == NULL){
		kPrintf("Memory Allocate Fail~!!\n");
		return;
	}

	kMemSet(pbBuffer, 0, FILESYSTEM_CLUSTERSIZE);

	kPrintf("====>>>> File I/O Performance Test Start\n");

	//----------------------------------------------------------------------------------------------------
	// 1-1. 클러스터 단위로 파일을 순차적으로 쓰는 테스트
	//----------------------------------------------------------------------------------------------------
	kPrintf("1. Sequential Read/Write Test (Cluster -> 1MB)\n");

	// 기존 파일을 삭제하고 새로 생성
	remove("performance.txt");
	pstFile = fopen("performance.txt", "w");
	if(pstFile == NULL){
		kPrintf("File Open Fail~!!\n");
		kFreeMemory(pbBuffer);
		return;
	}

	qwLastTickCount = kGetTickCount();

	// 클러스터 단위로 쓰는 테스트
	for(i = 0; i < (dwClusterTestFileSize / FILESYSTEM_CLUSTERSIZE); i++){
		if(fwrite(pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile) != FILESYSTEM_CLUSTERSIZE){
			kPrintf("File Write Fail~!!\n");
			fclose(pstFile);
			remove("performance.txt");
			kFreeMemory(pbBuffer);
			return;
		}
	}

	// 테스트 시간 출력
	kPrintf("    - Write : %d ms\n", kGetTickCount() - qwLastTickCount);

	//----------------------------------------------------------------------------------------------------
	// 1-2. 클러스터 단위로 파일을 순차적으로 읽는 테스트
	//----------------------------------------------------------------------------------------------------

	// 파일의 처음으로 이동
	fseek(pstFile, 0, SEEK_SET);

	qwLastTickCount = kGetTickCount();

	// 클러스터 단위로 읽는 테스트
	for(i = 0; i < (dwClusterTestFileSize / FILESYSTEM_CLUSTERSIZE); i++){
		if(fread(pbBuffer, 1, FILESYSTEM_CLUSTERSIZE, pstFile) != FILESYSTEM_CLUSTERSIZE){
			kPrintf("File Read Fail~!!\n");
			fclose(pstFile);
			remove("performance.txt");
			kFreeMemory(pbBuffer);
			return;
		}
	}

	// 테스트 시간 출력
	kPrintf("    - Read  : %d ms\n", kGetTickCount() - qwLastTickCount);

	//----------------------------------------------------------------------------------------------------
	// 2-1. 바이트 단위로 파일을 순차적으로 쓰는 테스트
	//----------------------------------------------------------------------------------------------------
	kPrintf("2. Sequential Read/Write Test (Byte -> 16KB)\n");

	// 기존 파일을 삭제하고 새로 생성
	fclose(pstFile);
	remove("performance.txt");
	pstFile = fopen("performance.txt", "w");
	if(pstFile == NULL){
		kPrintf("File Open Fail~!!\n");
		kFreeMemory(pbBuffer);
		return;
	}

	qwLastTickCount = kGetTickCount();

	// 바이트 단위로 쓰는 테스트
	for(i = 0; i < dwOneByteTestFileSize; i++){
		if(fwrite(pbBuffer, 1, 1, pstFile) != 1){
			kPrintf("File Write Fail~!!\n");
			fclose(pstFile);
			remove("performance.txt");
			kFreeMemory(pbBuffer);
			return;
		}
	}

	// 테스트 시간 출력
	kPrintf("    - Write : %d ms\n", kGetTickCount() - qwLastTickCount);

	//----------------------------------------------------------------------------------------------------
	// 2-2. 바이트 단위로 파일을 순차적으로 읽는 테스트
	//----------------------------------------------------------------------------------------------------

	// 파일의 처음으로 이동
	fseek(pstFile, 0, SEEK_SET);

	qwLastTickCount = kGetTickCount();

	// 바이트 단위로 읽는 테스트
	for(i = 0; i < dwOneByteTestFileSize; i++){
		if(fread(pbBuffer, 1, 1, pstFile) != 1){
			kPrintf("File Read Fail~!!\n");
			fclose(pstFile);
			remove("performance.txt");
			kFreeMemory(pbBuffer);
			return;
		}
	}

	// 테스트 시간 출력
	kPrintf("    - Read  : %d ms\n", kGetTickCount() - qwLastTickCount);

	// 파일을 닫고, 삭제 후, 메모리 해제
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

