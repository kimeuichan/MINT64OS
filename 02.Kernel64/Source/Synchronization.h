#ifndef __SYNCHRONIZATION_H__
#define __SYNCHRONIZATION_H__

#include "Types.h"

/***** 구조체 정의 *****/
#pragma pack(push, 1)

typedef struct kMutexStruct{
	volatile QWORD qwTaskID;    // 잠금을 수행한 태스크 ID
	volatile DWORD dwLockCount; // 잠금 횟수
	volatile BOOL bLockFlag;    // 잠금 플래그(잠금 여부)
	BYTE vbPadding[3];          // 패딩 바이트(자료구조의 크기를 8바이트 단위로 맞추려고 추가한 필드)
} MUTEX;

#pragma pack(pop)

/***** 함수 정의 *****/
BOOL kLockForSystemData(void);                  // 잠금        : 태스크와 인터럽트간의 동기화
void kUnlockForSystemData(BOOL bInterruptFlag); // 잠금 해제 : 태스크와 인터럽트간의 동기화
void kInitializeMutex(MUTEX* pstMutex);         // 뮤텍스 초기화
void kLock(MUTEX* pstMutex);                    // 잠금        : 태스크와 태스크간의 동기화(뮤텍스 이용)
void kUnlock(MUTEX* pstMutex);                  // 잠금 해제 : 태스크와 태스크간의 동기화(뮤텍스 이용)

#endif // __SYNCHRONIZATION_H__
