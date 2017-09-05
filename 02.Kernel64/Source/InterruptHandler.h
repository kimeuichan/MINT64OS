#ifndef __INTERRUPT_HANDLER_H__
#define __INTERRUPT_HANDLER_H__

#include "Types.h"

//****************************************************************************************************
// <<예외/인터럽트 메세지 출력 위치>>
// 1. 상단 중앙에 크게 출력             : kCommonExceptionHandler(Exception)
// 2. 첫번째 행의 첫번째 위치에 출력 : kKeyboardHandler(INT), kDeviceNotAvailableHandler(EXC)
// 3. 첫번째 행의 두번째 위치에 출력 : kHDDHandler(INT)
// 4. 첫번째 행의 마지막 위치에 출력 : kTimerHandler(INT), kCommonInterruptHandler(INT)
//****************************************************************************************************

/***** 함수 정의 *****/
void kCommonExceptionHandler(int iVectorNumber, QWORD qwErrorCode);
void kCommonInterruptHandler(int iVectorNumber);
void kKeyboardHandler(int iVectorNumber);
void kTimerHandler(int iVectorNumber);
void kDeviceNotAvailableHandler(int iVectorNumber);
void kHDDHandler(int iVectorNumber);

#endif // __INTERRUPT_HANDLER_H__
