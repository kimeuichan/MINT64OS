#include "InterruptHandler.h"
#include "PIC.h"
#include "Keyboard.h"
#include "Console.h"
#include "Utility.h"
#include "Task.h"
#include "Descriptor.h"
#include "AssemblyUtility.h"
#include "HardDisk.h"

static INTERRUPTMANAGER gs_stInterruptManager;

void kInitializeHandler(void){
	kMemSet(&gs_stInterruptManager, 0, sizeof(gs_stInterruptManager));
}

// 인터럽트 처리 모드
void kSetSymmetricIOMode(BOOL bSymmetricIOMode){
	gs_stInterruptManager.bSymmetricIOMode = bSymmetricIOMode;
}

void kSetInterruptLoadBalancing(BOOL bUseLoadBalancing){
	gs_stInterruptManager.bUseLoadBalancing = bUseLoadBalancing;
}

void kIncreaseInterruptCount(int iIRQ){
	gs_stInterruptManager.vvqwCoreInterruptCount[kGetAPICID()][iIRQ]++;
}

void kSendEOI(int iIRQ){
	if(gs_stInterruptManager.bSymmetricIOMode == FALSE)
		kSendEOIToPIC(iIRQ);
	else
		kSendEOIToLocalAPIC(iIRQ);
}

INTERRUPTMANAGER* kGetInterruptManager(void){
	return &gs_stInterruptManager;
}

void kProcessLoadBalancing(int iIRQ){
	QWORD qwMinCount = 0xffffffffffffffff;
	int iMinCountCoreIndex;
	int iCoreCount;
	int i;
	BOOL bResetCount = FALSE;
	BYTE bAPICID;

	bAPICID = kGetAPICID();

	// 부하 분산기능이 꺼져 있거나, 부하 분산할 시점이 아니면 할 필요가 없음
	if( (gs_stInterruptManager.vvqwCoreInterruptCount[bAPICID][iIRQ] == 0) || ((gs_stInterruptManager.vvqwCoreInterruptCount[bAPICID][iIRQ] % INTERRUPT_LOADBALANCINGDIVIDOR) != 0) || (gs_stInterruptManager.bUseLoadBalancing == FALSE))
		return;

	iMinCountCoreIndex = 0;
	iCoreCount = kGetProcessorCount();
	for(i=0; i<iCoreCount; i++){
		if( (gs_stInterruptManager.vvqwCoreInterruptCount[i][iIRQ] < qwMinCount)){
			qwMinCount = gs_stInterruptManager.vvqwCoreInterruptCount[i][iIRQ];
			iMinCountCoreIndex = i;
		}
		// 전체 카운트가 거의 최대값에 근접했다면 나주에 카운트를 모두 0으로 설정
		else if(gs_stInterruptManager.vvqwCoreInterruptCount[i][iIRQ] >= 0xfffffffffffffffe)
			bResetCount = TRUE;
	}

	// I/O 리다이렉션 테이블을 변경하여 가장 인터럽트를 처리한 횟수가 작은 로컬 APIC로 전달
	kRoutingIRQToAPICID(iIRQ, iMinCountCoreIndex);

	if(bResetCount == TRUE){
		for(i=0; i<i<iCoreCount; i++)
			gs_stInterruptManager.vvqwCoreInterruptCount[i][iIRQ] =  0;
	}
}

void kCommonExceptionHandler(int iVectorNumber, QWORD qwErrorCode){
	char vcBuffer[3] = {0, };

	// º¤ÅÍ ¹øÈ£ ¼³Á¤ (2ÀÚ¸® Á¤¼ö)
	vcBuffer[0] = '0' + iVectorNumber / 10;
	vcBuffer[1] = '0' + iVectorNumber % 10;

	kPrintStringXY( 0, 0, "====================================================" );
    kPrintStringXY( 0, 1, "                 Exception Occur~!!!!               " );
    // 예외 벡터와 코어 ID, 에러코드를 출력
    kSPrintf( vcBuffer,   "     Vector:%d     Core ID:0x%X     ErrorCode:0x%X  ", 
            iVectorNumber, kGetAPICID(), 1 );            
    kPrintStringXY( 0, 2, vcBuffer );    
    // 태스크 ID를 출력
    kPrintStringXY( 0, 3, "====================================================" );

	while(1);
}

void kCommonInterruptHandler(int iVectorNumber){
	char vcBuffer[] = "[INT:  , ]";
	static int g_iCommonInterruptCount = 0;
	int iIRQ;

	//====================================================================================================
	// ÀÎÅÍ·´Æ®°¡ ¹ß»ýÇßÀ½À» ¾Ë¸®·Á°í ¸Þ¼¼Áö¸¦ Ãâ·ÂÇÏ´Â ºÎºÐ
	// º¤ÅÍ ¹øÈ£ ¼³Á¤ (2ÀÚ¸® Á¤¼ö)
	vcBuffer[5] = '0' + iVectorNumber / 10;
	vcBuffer[6] = '0' + iVectorNumber % 10;

	// ¹ß»ý È½¼ö ¼³Á¤ (1ÀÚ¸® Á¤¼ö)
	g_iCommonInterruptCount = (g_iCommonInterruptCount + 1) % 10;
	vcBuffer[8] = '0' + g_iCommonInterruptCount;

	// Ã¹¹øÂ° ÇàÀÇ ¸¶Áö¸· À§Ä¡¿¡ Ãâ·Â
	kPrintStringXY(70, 0, vcBuffer);
	//====================================================================================================

	iIRQ = iVectorNumber - PIC_IRQSTARTVECTOR;

	kSendEOI(iIRQ);

	kIncreaseInterruptCount(iIRQ);

	kProcessLoadBalancing(iIRQ);

}

void kKeyboardHandler(int iVectorNumber){
	char vcBuffer[] = "[INT:  , ]";
	static int g_iKeyboardInterruptCount = 0;
	BYTE bScanCode;
	int iIRQ;

	//====================================================================================================
	// ÀÎÅÍ·´Æ®°¡ ¹ß»ýÇßÀ½À» ¾Ë¸®·Á°í ¸Þ¼¼Áö¸¦ Ãâ·ÂÇÏ´Â ºÎºÐ
	// º¤ÅÍ ¹øÈ£ ¼³Á¤ (2ÀÚ¸® Á¤¼ö)
	vcBuffer[5] = '0' + iVectorNumber / 10;
	vcBuffer[6] = '0' + iVectorNumber % 10;

	// ¹ß»ý È½¼ö ¼³Á¤ (1ÀÚ¸® Á¤¼ö)
	g_iKeyboardInterruptCount = (g_iKeyboardInterruptCount + 1) % 10;
	vcBuffer[8] = '0' + g_iKeyboardInterruptCount;

	// Ã¹¹øÂ° ÇàÀÇ Ã¹¹øÂ° À§Ä¡¿¡ Ãâ·Â
	kPrintStringXY(0, 0, vcBuffer);
	//====================================================================================================

	if(kIsOutputBufferFull() == TRUE){
		bScanCode = kGetKeyboardScanCode();
		kConvertScanCodeAndPutQueue(bScanCode);
	}

	iIRQ = iVectorNumber - PIC_IRQSTARTVECTOR;

	kSendEOI(iIRQ);

	kIncreaseInterruptCount(iIRQ);

	kProcessLoadBalancing(iIRQ);
}

void kTimerHandler(int iVectorNumber){
	char vcBuffer[] = "[INT:  , ]";
	static int g_iTimerInterruptCount = 0;
	int iIRQ;
	BYTE bCurrentAPICID;

	//====================================================================================================
	// ÀÎÅÍ·´Æ®°¡ ¹ß»ýÇßÀ½À» ¾Ë¸®·Á°í ¸Þ¼¼Áö¸¦ Ãâ·ÂÇÏ´Â ºÎºÐ
	// º¤ÅÍ ¹øÈ£ ¼³Á¤ (2ÀÚ¸® Á¤¼ö)
	vcBuffer[5] = '0' + iVectorNumber / 10;
	vcBuffer[6] = '0' + iVectorNumber % 10;

	// ¹ß»ý È½¼ö ¼³Á¤ (1ÀÚ¸® Á¤¼ö)
	g_iTimerInterruptCount = (g_iTimerInterruptCount + 1) % 10;
	vcBuffer[8] = '0' + g_iTimerInterruptCount;

	// Ã¹¹øÂ° ÇàÀÇ ¸¶Áö¸· À§Ä¡¿¡ Ãâ·Â
	kPrintStringXY(70, 0, vcBuffer);
	//====================================================================================================

	iIRQ = iVectorNumber - PIC_IRQSTARTVECTOR;

	// EOI 전송
	kSendEOI(iIRQ);

	// 인터럽트 발생 횟수를 업데이트
	kIncreaseInterruptCount(iIRQ);

	// IRQ 0 인터럽트는 Booststrap Processor만 처리
	bCurrentAPICID = kGetAPICID();
	if(bCurrentAPICID == 0){
		g_qwTickCount++;
	}

	// 태스크가 사용한 프로세서의 시간을 줄임
	kDecreaseProcessorTime(bCurrentAPICID);

	if(kIsProcessorTimeExpired(bCurrentAPICID) == TRUE){
		kScheduleInInterrupt();
	}
}

void kDeviceNotAvailableHandler(int iVectorNumber){
	TCB* pstFPUTask;       // ¸¶Áö¸· FPU »ç¿ë ÅÂ½ºÅ©
	TCB* pstCurrentTask;   // ÇöÀç ÅÂ½ºÅ©
	QWORD qwLastFPUTaskID; // ¸¶Áö¸· FPU »ç¿ë ÅÂ½ºÅ© ID
	BYTE bCurrentAPICID;

	bCurrentAPICID = kGetAPICID();

	//====================================================================================================
	// FPU ¿¹¿Ü°¡ ¹ß»ýÇßÀ½À» ¾Ë¸®·Á°í ¸Þ¼¼Áö¸¦ Ãâ·ÂÇÏ´Â ºÎºÐ
	char vcBuffer[] = "[EXC:  , ]";
	static int g_iFPUExceptionCount = 0;

	// º¤ÅÍ ¹øÈ£ ¼³Á¤ (2ÀÚ¸® Á¤¼ö)
	vcBuffer[5] = '0' + iVectorNumber / 10;
	vcBuffer[6] = '0' + iVectorNumber % 10;

	// ¹ß»ý È½¼ö ¼³Á¤ (1ÀÚ¸® Á¤¼ö)
	g_iFPUExceptionCount = (g_iFPUExceptionCount + 1) % 10;
	vcBuffer[8] = '0' + g_iFPUExceptionCount;

	// Ã¹¹øÂ° ÇàÀÇ Ã¹¹øÂ° À§Ä¡¿¡ Ãâ·Â
	kPrintStringXY(0, 0, vcBuffer);
	//====================================================================================================

	// CR0 컨트롤 레지스터의 TS 비트를 0으로 설정
	kClearTS();

	qwLastFPUTaskID = kGetLastFPUUsedTaskID(bCurrentAPICID);
	pstCurrentTask = kGetRunningTask(bCurrentAPICID);

	// ¸¶Áö¸· FPU »ç¿ë ÅÂ½ºÅ©°¡ ÇöÀç ÅÂ½ºÅ©ÀÎ °æ¿ì, FPU ÄÜÅØ½ºÆ®ÀÇ ÀúÀå ¹× º¹¿øÀÌ ÇÊ¿ä ¾ø°í, ÇöÀç FPU ÄÜÅØ½ºÆ®¸¦ ±×´ë·Î »ç¿ëÇÏ¸é µÇ¹Ç·Î ÇÔ¼ö Á¾·á
	if(qwLastFPUTaskID == pstCurrentTask->stLink.qwID){
		return;

	// ¸¶Áö¸· FPU »ç¿ë ÅÂ½ºÅ©°¡ Á¸Àç ÇÏ´Â °æ¿ì, ÇöÀç FPU ÄÜÅØ½ºÆ®¸¦ ¸Þ¸ð¸®¿¡ ÀúÀå
	}else if(qwLastFPUTaskID != TASK_INVALIDID){
		pstFPUTask = kGetTCBInTCBPool(GETTCBOFFSET(qwLastFPUTaskID));
		if((pstFPUTask != NULL) && (pstFPUTask->stLink.qwID == qwLastFPUTaskID)){
			kSaveFPUContext(pstFPUTask->vqwFPUContext);
		}
	}

	// ÇöÀç ÅÂ½ºÅ©°¡ FPU¸¦ »ç¿ëÇÑ ÀûÀÌ ¾ø´Â °æ¿ì, FPU ÃÊ±âÈ­
	if(pstCurrentTask->bFPUUsed == FALSE){
		kInitializeFPU();
		pstCurrentTask->bFPUUsed = TRUE;

	// ÇöÀç ÅÂ½ºÅ©°¡ FPU¸¦ »ç¿ëÇÑ ÀûÀÌ ÀÖ´Â °æ¿ì, ÇöÀç ÅÂ½ºÅ©ÀÇ FPU ÄÜÅØ½ºÆ®¸¦ ¸Þ¸ð¸®¿¡¼­ º¹¿ø
	}else{
		kLoadFPUContext(pstCurrentTask->vqwFPUContext);
	}

	// ¸¶Áö¸· FPU »ç¿ë ÅÂ½ºÅ©¸¦ ÇöÀç ÅÂ½ºÅ©·Î º¯°æ
	kSetLastFPUUsedTaskID(bCurrentAPICID, pstCurrentTask->stLink.qwID);
}

void kHDDHandler(int iVectorNumber){
	char vcBuffer[] = "[INT:  , ]";
	static int g_iHDDInterruptCount = 0;
	int iIRQ;

	//====================================================================================================
	// ÀÎÅÍ·´Æ®°¡ ¹ß»ýÇßÀ½À» ¾Ë¸®·Á°í ¸Þ¼¼Áö¸¦ Ãâ·ÂÇÏ´Â ºÎºÐ
	// º¤ÅÍ ¹øÈ£ ¼³Á¤ (2ÀÚ¸® Á¤¼ö)
	vcBuffer[5] = '0' + iVectorNumber / 10;
	vcBuffer[6] = '0' + iVectorNumber % 10;

	// ¹ß»ý È½¼ö ¼³Á¤ (1ÀÚ¸® Á¤¼ö)
	g_iHDDInterruptCount = (g_iHDDInterruptCount + 1) % 10;
	vcBuffer[8] = '0' + g_iHDDInterruptCount;

	// Ã¹¹øÂ° ÇàÀÇ µÎ¹øÂ° À§Ä¡¿¡ Ãâ·Â
	kPrintStringXY(10, 0, vcBuffer);
	//====================================================================================================

	// Ã¹¹øÂ° PATA Æ÷Æ®ÀÇ ÀÎÅÍ·´Æ® º¤ÅÍ(IRQ14) Ã³¸®
	iIRQ = iVectorNumber - PIC_IRQSTARTVECTOR;
	if(iIRQ == 14){
		// Ã¹¹øÂ° PATA Æ÷Æ®ÀÇ  ÀÎÅÍ·´Æ® ÇÃ·¡±×¸¦ TRUE·Î ¼³Á¤
		kSetHDDInterruptFlag(TRUE, TRUE);

	// Ã¹¹øÂ° PATA Æ÷Æ®ÀÇ ÀÎÅÍ·´Æ® º¤ÅÍ(IRQ15) Ã³¸®
	}else{
		// µÎ¹øÂ° PATA Æ÷Æ®ÀÇ  ÀÎÅÍ·´Æ® ÇÃ·¡±×¸¦ TRUE·Î ¼³Á¤
		kSetHDDInterruptFlag(FALSE, TRUE);
	}
	
	kSendEOI(iIRQ);

	kIncreaseInterruptCount(iIRQ);

	kProcessLoadBalancing(iIRQ);
}
