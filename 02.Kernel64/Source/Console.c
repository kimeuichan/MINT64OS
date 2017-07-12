#include <stdarg.h>
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"

/***** 전역 변수 정의 *****/
static CONSOLEMANAGER gs_stConsoleManager = {0, };

void kInitializeConsole(int iX, int iY){
	kMemSet(&gs_stConsoleManager, 0, sizeof(gs_stConsoleManager));
	kSetCursor(iX, iY);
}

void kSetCursor(int iX, int iY){
	int iLinearValue;

	iLinearValue = iY * CONSOLE_WIDTH + iX;

	// CRTC 컨트롤 어드레스 레지스터(0x3D4)에 상위 커서 위치 레지스터 선택 커맨드(0x0E)를 전송
	kOutPortByte(VGA_PORT_INDEX, VGA_INDEX_UPPERCURSOR);

	// CRTC 컨트롤 데이터 레지스터(0x3D5)에 커서의 상위 바이트를 전송
	kOutPortByte(VGA_PORT_DATA, iLinearValue >> 8);

	// CRTC 컨트롤 어드레스 레지스터(0x3D4)에 하위 커서 위치 레지스터 선택 커맨드(0x0F)를 전송
	kOutPortByte(VGA_PORT_INDEX, VGA_INDEX_LOWERCURSOR);

	// CRTC 컨트롤 데이터 레지스터(0x3D5)에 커서의 하위 바이트를 전송
	kOutPortByte(VGA_PORT_DATA, iLinearValue & 0xFF);

	gs_stConsoleManager.iCurrentPrintOffset = iLinearValue;
}

void kGetCursor(int* piX, int* piY){
	*piX = gs_stConsoleManager.iCurrentPrintOffset % CONSOLE_WIDTH;
	*piY = gs_stConsoleManager.iCurrentPrintOffset / CONSOLE_WIDTH;
}

void kPrintf(const char* pcFormatString, ...){
	va_list ap;
	char vcBuffer[1024];
	int iNextPrintOffset;

	va_start(ap, pcFormatString);
	kVSPrintf(vcBuffer, pcFormatString, ap);
	va_end(ap);

	iNextPrintOffset = kConsolePrintString(vcBuffer);

	kSetCursor(iNextPrintOffset % CONSOLE_WIDTH, iNextPrintOffset / CONSOLE_WIDTH);
}

int kConsolePrintString(const char* pcBuffer){
	CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
	int i, j;
	int iLength;
	int iPrintOffset;

	iPrintOffset = gs_stConsoleManager.iCurrentPrintOffset;

	iLength = kStrLen(pcBuffer);

	for(i = 0; i < iLength; i++){
		// 줄바꿈 처리
		if(pcBuffer[i] == '\n'){
			// 출력 위치를 80의 배수 위치로 이동 (다음 라인의 첫번째 위치로 이동)
			iPrintOffset += (CONSOLE_WIDTH - (iPrintOffset % CONSOLE_WIDTH));

		// 탭 처리
		}else if(pcBuffer[i] == '\t'){
			// 출력 위치를 8의 배수 위치로 이동 (다음 탭의 첫번째 위치로 이동)
			iPrintOffset += (8 - (iPrintOffset % 8));

		// 일반 문자열 처리
		}else{
			pstScreen[iPrintOffset].bCharacter = pcBuffer[i];
			pstScreen[iPrintOffset].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
			iPrintOffset++;
		}

		// 스크롤 처리(출력 위치가 화면의 최대값(80*25)을 벗어난 경우)
		if(iPrintOffset > (CONSOLE_WIDTH * CONSOLE_HEIGHT)){

			// 비디오 메모리 전체(두번째 줄부터 마지막 줄까지)를 한 줄 위로 복사
			kMemCpy(CONSOLE_VIDEOMEMORYADDRESS
				   ,CONSOLE_VIDEOMEMORYADDRESS + (CONSOLE_WIDTH * sizeof(CHARACTER))
				   ,(CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH * sizeof(CHARACTER));

			// 마지막 줄은 공백으로 채움
			for(j = ((CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH); j < (CONSOLE_HEIGHT * CONSOLE_WIDTH); j++){
				pstScreen[j].bCharacter = ' ';
				pstScreen[j].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;

			}

			// 출력 위치를 마지막 줄의 첫번째 위치로 이동
			iPrintOffset = (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH;
		}
	}

	return iPrintOffset;
}

void kClearScreen(void){
	CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
	int i;

	for(i = 0; i < (CONSOLE_HEIGHT * CONSOLE_WIDTH); i++){
		pstScreen[i].bCharacter = ' ';
		pstScreen[i].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
	}

	kSetCursor(0, 0);
}

BYTE kGetCh(void){
	KEYDATA stData;

	// 키가 눌러질 때까지 대기
	while(1){

		// 키 큐에 데이터가 수신될 때까지 대기
		while(kGetKeyFromKeyQueue(&stData) == FALSE){
			kSchedule();
		}

		// 키 큐에 데이터가 수신되면 아스키 코드를 반환
		if(stData.bFlags & KEY_FLAGS_DOWN){
			return stData.bASCIICode;
		}
	}
}

void kPrintStringXY(int iX, int iY, const char* pcString){
	CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
	int i;

	pstScreen += (iY * CONSOLE_WIDTH) + iX;

	for(i = 0; pcString[i] != NULL; i++){
		pstScreen[i].bCharacter = pcString[i];
		pstScreen[i].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
	}
}