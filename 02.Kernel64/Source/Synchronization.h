#ifndef __SYNCHRONIZATION_H__
#define __SYNCHRONIZATION_H__

#include "Types.h"

#pragma pack(push, 1)

typedef struct MutextStruct{
	volatile QWORD _TaskID;
	volatile DWORD _LockCount;
	volatile BOOL _LockFlag;

	BYTE Pandding[3];
} MUTEX;

#pragma pack(pop)

BOOL kLockForSystemData(void);
BOOL kUnlockForSystemData(BOOL bInterruptFlag);

void kInitializeMutex(MUTEX* pstMutex);
void kLock(MUTEX* pstMutex);
void kUnlock(MUTEX* pstMutex);

#endif