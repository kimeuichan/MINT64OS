#ifndef __INTERRUPT_HANDLER_H__
#define __INTERRUPT_HANDLER_H__

#include "Types.h"
#include "MultiProcessor.h"

#define INTERRUPT_MAXVECTORCOUNT 16
#define INTERRUPT_LOADBALANCINGDIVIDOR	10

typedef struct kInterruptManagerStruct{
	// 인터럽트 처리 횟수 코어 최대 개수 X 최대 벡터 수 
	QWORD vvqwCoreInterruptCount[MAXPROCESSORCOUNT][INTERRUPT_MAXVECTORCOUNT];

	// 인터럽트 분산 기능 사용 여부
	BOOL bUseLoadBalancing;

	// 대칭 I/O 사용 여부
	BOOL bSymmetricIOMode;
} INTERRUPTMANAGER;

//****************************************************************************************************
// <<¿¹¿Ü/ÀÎÅÍ·´Æ® ¸Þ¼¼Áö Ãâ·Â À§Ä¡>>
// 1. »ó´Ü Áß¾Ó¿¡ Å©°Ô Ãâ·Â             : kCommonExceptionHandler(Exception)
// 2. Ã¹¹øÂ° ÇàÀÇ Ã¹¹øÂ° À§Ä¡¿¡ Ãâ·Â : kKeyboardHandler(INT), kDeviceNotAvailableHandler(EXC)
// 3. Ã¹¹øÂ° ÇàÀÇ µÎ¹øÂ° À§Ä¡¿¡ Ãâ·Â : kHDDHandler(INT)
// 4. Ã¹¹øÂ° ÇàÀÇ ¸¶Áö¸· À§Ä¡¿¡ Ãâ·Â : kTimerHandler(INT), kCommonInterruptHandler(INT)
//****************************************************************************************************

/***** ÇÔ¼ö Á¤ÀÇ *****/
void kCommonExceptionHandler(int iVectorNumber, QWORD qwErrorCode);
void kCommonInterruptHandler(int iVectorNumber);
void kKeyboardHandler(int iVectorNumber);
void kTimerHandler(int iVectorNumber);
void kDeviceNotAvailableHandler(int iVectorNumber);
void kHDDHandler(int iVectorNumber);
void kInitializeHandler(void);
void kSetSymmetricIOMode(BOOL bSymmetricIOMode);
void kSetInterruptLoadBalancing(BOOL bUseLoadBalancing);
void kIncreaseInterruptCount(int iIRQ);
void kSendEOI(int iIRQ);
INTERRUPTMANAGER* kGetInterruptManager(void);
void kProcessLoadBalancing(int iIRQ);

#endif // __INTERRUPT_HANDLER_H__
