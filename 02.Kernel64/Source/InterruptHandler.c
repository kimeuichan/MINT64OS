#include "InterruptHandler.h"
#include "PIC.h"
// #include "Utility.h"
#include "Keyboard.h"

void kCommonExceptionHandler(int iVectorNumber, QWORD qwErrorCode){
	char vcBuffer[3] = {0, };

	// 벡터 번호 설정 (2자리 정수)
	vcBuffer[0] = '0' + iVectorNumber / 10;
	vcBuffer[1] = '0' + iVectorNumber % 10;

	kPrintString(0, 0, "==================================================");
	kPrintString(0, 1, "               Exception Occur~!!                 ");
	kPrintString(0, 2, "                  Vector:                         ");
	kPrintString(27, 2, vcBuffer); // "Vector:" 문자열 옆에 출력
	kPrintString(0, 3, "==================================================");

	while(1);
}

void kCommonInterruptHandler(int iVectorNumber){
	char vcBuffer[] = "[INT:  , ]";
	static int g_iCommonInterruptCount = 0;

	//====================================================================================================
	// 인터럽트가 발생했음을 알리려고 메세지를 출력하는 부분
	// 벡터 번호 설정 (2자리 정수)
	vcBuffer[5] = '0' + iVectorNumber / 10;
	vcBuffer[6] = '0' + iVectorNumber % 10;

	// 발생 횟수 설정 (1자리 정수)
	g_iCommonInterruptCount = (g_iCommonInterruptCount + 1) % 10;
	vcBuffer[8] = '0' + g_iCommonInterruptCount;

	// 화면 오른쪽 위에 출력
	kPrintString(70, 0, vcBuffer);
	//====================================================================================================

	kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);
}

void kKeyboardHandler(int iVectorNumber){
	char vcBuffer[] = "[INT:  , ]";
	static int g_iKeyboardInterruptCount = 0;
	BYTE bTemp;

	//====================================================================================================
	// 인터럽트가 발생했음을 알리려고 메세지를 출력하는 부분
	// 벡터 번호 설정 (2자리 정수)
	vcBuffer[5] = '0' + iVectorNumber / 10;
	vcBuffer[6] = '0' + iVectorNumber % 10;

	// 발생 횟수 설정 (1자리 정수)
	g_iKeyboardInterruptCount = (g_iKeyboardInterruptCount + 1) % 10;
	vcBuffer[8] = '0' + g_iKeyboardInterruptCount;

	// 화면 왼쪽 위에 출력
	kPrintString(0, 0, vcBuffer);
	//====================================================================================================

	if(kIsOutputBufferFull() == TRUE){
		bTemp = kGetKeyboardScanCode();
		kConvertScanCodeAndPutQueue(bTemp);
	}
	kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);
}
