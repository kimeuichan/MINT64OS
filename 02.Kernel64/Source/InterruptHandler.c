#include "InterruptHandler.h"
#include "PIC.h"
#include "Keyboard.h"
#include "Console.h"
#include "Utility.h"
#include "Task.h"
#include "Descriptor.h"

void kCommonExceptionHandler(int iVectorNumber, QWORD qwErrorCode){
	char vcBuffer[3] = {0, };

	// 벡터 번호 설정 (2자리 정수)
	vcBuffer[0] = '0' + iVectorNumber / 10;
	vcBuffer[1] = '0' + iVectorNumber % 10;

	kPrintStringXY(0, 0, "==================================================");
	kPrintStringXY(0, 1, "               Exception Occur~!!                 ");
	kPrintStringXY(0, 2, "                  Vector: [  ]                    ");
	kPrintStringXY(27, 2, vcBuffer); // "Vector:" 문자열 옆에 출력
	kPrintStringXY(0, 3, "==================================================");

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

	// 화면 오른쪽 위의 첫번째 행에 출력
	kPrintStringXY(70, 0, vcBuffer);
	//====================================================================================================

	kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);
}

void kKeyboardHandler(int iVectorNumber){
	char vcBuffer[] = "[INT:  , ]";
	static int g_iKeyboardInterruptCount = 0;
	BYTE bScanCode;

	//====================================================================================================
	// 인터럽트가 발생했음을 알리려고 메세지를 출력하는 부분
	// 벡터 번호 설정 (2자리 정수)
	vcBuffer[5] = '0' + iVectorNumber / 10;
	vcBuffer[6] = '0' + iVectorNumber % 10;

	// 발생 횟수 설정 (1자리 정수)
	g_iKeyboardInterruptCount = (g_iKeyboardInterruptCount + 1) % 10;
	vcBuffer[8] = '0' + g_iKeyboardInterruptCount;

	// 화면 왼쪽 위의 첫번째 행에 출력
	kPrintStringXY(0, 0, vcBuffer);
	//====================================================================================================

	if(kIsOutputBufferFull() == TRUE){
		bScanCode = kGetKeyboardScanCode();
		kConvertScanCodeAndPutQueue(bScanCode);
	}

	kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);
}

void kTimerHandler(int iVectorNumber){
	char vcBuffer[] = "[INT:  , ]";
	static int g_iTimerInterruptCount = 0;

	//====================================================================================================
	// 인터럽트가 발생했음을 알리려고 메세지를 출력하는 부분
	// 벡터 번호 설정 (2자리 정수)
	vcBuffer[5] = '0' + iVectorNumber / 10;
	vcBuffer[6] = '0' + iVectorNumber % 10;

	// 발생 횟수 설정 (1자리 정수)
	g_iTimerInterruptCount = (g_iTimerInterruptCount + 1) % 10;
	vcBuffer[8] = '0' + g_iTimerInterruptCount;

	// 화면 오른쪽 위의 첫번째 행에 출력
	kPrintStringXY(70, 0, vcBuffer);
	//====================================================================================================

	kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);

	g_qwTickCount++;

	kDecreaseProcessorTime();

	if(kIsProcessorTimeExpired() == TRUE){
		kScheduleInInterrupt();
	}
}