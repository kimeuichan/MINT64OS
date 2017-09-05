#include "InterruptHandler.h"
#include "PIC.h"
#include "Keyboard.h"
#include "Console.h"
#include "Utility.h"
#include "Task.h"
#include "Descriptor.h"
#include "AssemblyUtility.h"
#include "HardDisk.h"

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

	// 첫번째 행의 마지막 위치에 출력
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

	// 첫번째 행의 첫번째 위치에 출력
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

	// 첫번째 행의 마지막 위치에 출력
	kPrintStringXY(70, 0, vcBuffer);
	//====================================================================================================

	kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);

	g_qwTickCount++;

	kDecreaseProcessorTime();

	if(kIsProcessorTimeExpired() == TRUE){
		kScheduleInInterrupt();
	}
}

void kDeviceNotAvailableHandler(int iVectorNumber){
	TCB* pstFPUTask;       // 마지막 FPU 사용 태스크
	TCB* pstCurrentTask;   // 현재 태스크
	QWORD qwLastFPUTaskID; // 마지막 FPU 사용 태스크 ID

	//====================================================================================================
	// FPU 예외가 발생했음을 알리려고 메세지를 출력하는 부분
	char vcBuffer[] = "[EXC:  , ]";
	static int g_iFPUExceptionCount = 0;

	// 벡터 번호 설정 (2자리 정수)
	vcBuffer[5] = '0' + iVectorNumber / 10;
	vcBuffer[6] = '0' + iVectorNumber % 10;

	// 발생 횟수 설정 (1자리 정수)
	g_iFPUExceptionCount = (g_iFPUExceptionCount + 1) % 10;
	vcBuffer[8] = '0' + g_iFPUExceptionCount;

	// 첫번째 행의 첫번째 위치에 출력
	kPrintStringXY(0, 0, vcBuffer);
	//====================================================================================================

	// CR0.TS=0 으로 설정
	kClearTS();

	qwLastFPUTaskID = kGetLastFPUUsedTaskID();
	pstCurrentTask = kGetRunningTask();

	// 마지막 FPU 사용 태스크가 현재 태스크인 경우, FPU 콘텍스트의 저장 및 복원이 필요 없고, 현재 FPU 콘텍스트를 그대로 사용하면 되므로 함수 종료
	if(qwLastFPUTaskID == pstCurrentTask->stLink.qwID){
		return;

	// 마지막 FPU 사용 태스크가 존재 하는 경우, 현재 FPU 콘텍스트를 메모리에 저장
	}else if(qwLastFPUTaskID != TASK_INVALIDID){
		pstFPUTask = kGetTCBInTCBPool(GETTCBOFFSET(qwLastFPUTaskID));
		if((pstFPUTask != NULL) && (pstFPUTask->stLink.qwID == qwLastFPUTaskID)){
			kSaveFPUContext(pstFPUTask->vqwFPUContext);
		}
	}

	// 현재 태스크가 FPU를 사용한 적이 없는 경우, FPU 초기화
	if(pstCurrentTask->bFPUUsed == FALSE){
		kInitializeFPU();
		pstCurrentTask->bFPUUsed = TRUE;

	// 현재 태스크가 FPU를 사용한 적이 있는 경우, 현재 태스크의 FPU 콘텍스트를 메모리에서 복원
	}else{
		kLoadFPUContext(pstCurrentTask->vqwFPUContext);
	}

	// 마지막 FPU 사용 태스크를 현재 태스크로 변경
	kSetLastFPUUsedTaskID(pstCurrentTask->stLink.qwID);
}

void kHDDHandler(int iVectorNumber){
	char vcBuffer[] = "[INT:  , ]";
	static int g_iHDDInterruptCount = 0;

	//====================================================================================================
	// 인터럽트가 발생했음을 알리려고 메세지를 출력하는 부분
	// 벡터 번호 설정 (2자리 정수)
	vcBuffer[5] = '0' + iVectorNumber / 10;
	vcBuffer[6] = '0' + iVectorNumber % 10;

	// 발생 횟수 설정 (1자리 정수)
	g_iHDDInterruptCount = (g_iHDDInterruptCount + 1) % 10;
	vcBuffer[8] = '0' + g_iHDDInterruptCount;

	// 첫번째 행의 두번째 위치에 출력
	kPrintStringXY(10, 0, vcBuffer);
	//====================================================================================================

	// 첫번째 PATA 포트의 인터럽트 벡터(IRQ14) 처리
	if((iVectorNumber - PIC_IRQSTARTVECTOR) == 14){
		// 첫번째 PATA 포트의  인터럽트 플래그를 TRUE로 설정
		kSetHDDInterruptFlag(TRUE, TRUE);

	// 첫번째 PATA 포트의 인터럽트 벡터(IRQ15) 처리
	}else{
		// 두번째 PATA 포트의  인터럽트 플래그를 TRUE로 설정
		kSetHDDInterruptFlag(FALSE, TRUE);
	}

	kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);
}
