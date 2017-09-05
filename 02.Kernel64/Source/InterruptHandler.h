#ifndef __INTERRUPT_HANDLER_H__
#define __INTERRUPT_HANDLER_H__

#include "Types.h"

//****************************************************************************************************
// <<����/���ͷ�Ʈ �޼��� ��� ��ġ>>
// 1. ��� �߾ӿ� ũ�� ���             : kCommonExceptionHandler(Exception)
// 2. ù��° ���� ù��° ��ġ�� ��� : kKeyboardHandler(INT), kDeviceNotAvailableHandler(EXC)
// 3. ù��° ���� �ι�° ��ġ�� ��� : kHDDHandler(INT)
// 4. ù��° ���� ������ ��ġ�� ��� : kTimerHandler(INT), kCommonInterruptHandler(INT)
//****************************************************************************************************

/***** �Լ� ���� *****/
void kCommonExceptionHandler(int iVectorNumber, QWORD qwErrorCode);
void kCommonInterruptHandler(int iVectorNumber);
void kKeyboardHandler(int iVectorNumber);
void kTimerHandler(int iVectorNumber);
void kDeviceNotAvailableHandler(int iVectorNumber);
void kHDDHandler(int iVectorNumber);

#endif // __INTERRUPT_HANDLER_H__
