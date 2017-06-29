#include "ConsoleShell.h"
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"
#include "PIT.h"
#include "RTC.h"
#include "AssemblyUtility.h"

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
void kHelp(const char* pcParameterBuffer){
	int i;
	int iCount;
	int iCursorX, iCursorY;
	int iLength, iMaxCommandLength = 0;

	kPrintf("==================================================\n");
	kPrintf("               MINT64 Shell Help                  \n");
	kPrintf("==================================================\n");

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
	}
}

void kCls(const char* pcParameterBuffer){
	kClearScreen();
	kSetCursor(0, 1); // 첫번째 줄에는 인터럽트를 표시하기 때문에, 두번째 줄로 커서 이동
}

void kShowTotalRAMSize(const char* pcParameterBuffer){
	kPrintf("Total RAM Size = %d MB\n", kGetTotalRAMSize());
}

void kStringToDecimalHexTest(const char* pcParameterBuffer){
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

void kShutdown(const char* pcParameterBuffer){
	kPrintf("System Shutdown Start...\n");

	// 키보드 컨트롤러를 통해 PC를 재부팅
	kPrintf("Press any key to reboot PC...");
	kGetCh();
	kReboot();
}

void kSetTimer(const char* pcParameterBuffer){
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

void kWaitUsingPIT(const char* pcParameterBuffer){
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

void kReadTimeStampCounter(const char* pcParameterBuffer){
	QWORD qwTSC;

	qwTSC = kReadTSC();
	kPrintf("Time Stamp Counter = %q\n", qwTSC);
}

void kMeasureProcessorSpeed(const char* pcParameterBuffer){
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
void kShowDateAndTime(const char* pcParameterBuffer){
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

void kTestTask(void){
	int i = 0;
	while(1){
		kPrintf("[%d] This message is from kTestTask, Press any key to siwtch kConsoleShell~!!\n", i++);
		kGetCh();

		kSwitchContext( &(gs_vstTask[1].stContext), &(gs_vstTask[0].stContext));
	}
}

// 태스크1
//	화면 테두리를 돌면서 문자를 출력
void kTestTask1(void){
	BYTE bData;
	int i = 0, iX = 0, iY = 0, iMargin;
	CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
	TCB* pstRunningTask;

	// 자신의 ID 얻어서 화면 오프셋으로 사용
	pstRunningTask = kGetRunningTask();
	iMargin = (pstRunningTask->stLink.qwID & 0xffffffff) % 10;

	while(1){
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

		kSchedule();
	}
}

// 태스크2
//	자신의 ID를 참고하여 특정 위치에 회전하는 바람개비를 출력
void kTestTask2(void){
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

void kCreateTestTask(const char* pcParameterBuffer){
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
				if(kCreateTask(0, (QWORD)kTestTask1) == NULL)
					break;
			}
			kPrintf("Task1 %d Created\n", i);
			break;
		case 2:
			for(i=0; i<kAToI(vcCount, 10); i++){
				if(kCreateTask(0, (QWORD)kTestTask2) == NULL)
					break;
			}
			kPrintf("Task2 %d Created\n", i);
			break;
	}
	
}