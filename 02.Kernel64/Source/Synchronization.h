#ifndef __SYNCHRONIZATION_H__
#define __SYNCHRONIZATION_H__

#include "Types.h"

/***** ±¸Á¶Ã¼ Á¤ÀÇ *****/
#pragma pack(push, 1)

typedef struct kMutexStruct{
	volatile QWORD qwTaskID;    // Àá±ÝÀ» ¼öÇàÇÑ ÅÂ½ºÅ© ID
	volatile DWORD dwLockCount; // Àá±Ý È½¼ö
	volatile BOOL bLockFlag;    // Àá±Ý ÇÃ·¡±×(Àá±Ý ¿©ºÎ)
	BYTE vbPadding[3];          // ÆÐµù ¹ÙÀÌÆ®(ÀÚ·á±¸Á¶ÀÇ Å©±â¸¦ 8¹ÙÀÌÆ® ´ÜÀ§·Î ¸ÂÃß·Á°í Ãß°¡ÇÑ ÇÊµå)
} MUTEX;

typedef struct kSpinLockStruct{
	// 로컬 APIC ID와 잠금을 수행한 횟수
	volatile DWORD dwLockCount;
	volatile BYTE bAPICID;

	// 잠금 플래그
	volatile BOOL bLockFlag;

	// 인터럽트 플래그
	volatile BOOL bInterruptFlag;

	BYTE vbPadding[1];
} SPINLOCK;

#pragma pack(pop)

/***** ÇÔ¼ö Á¤ÀÇ *****/
#if 0
BOOL kLockForSystemData(void);                  // Àá±Ý        : ÅÂ½ºÅ©¿Í ÀÎÅÍ·´Æ®°£ÀÇ µ¿±âÈ­
void kUnlockForSystemData(BOOL bInterruptFlag); // Àá±Ý ÇØÁ¦ : ÅÂ½ºÅ©¿Í ÀÎÅÍ·´Æ®°£ÀÇ µ¿±âÈ­
#endif
void kInitializeMutex(MUTEX* pstMutex);         // ¹ÂÅØ½º ÃÊ±âÈ­
void kLock(MUTEX* pstMutex);                    // Àá±Ý        : ÅÂ½ºÅ©¿Í ÅÂ½ºÅ©°£ÀÇ µ¿±âÈ­(¹ÂÅØ½º ÀÌ¿ë)
void kUnlock(MUTEX* pstMutex);                  // Àá±Ý ÇØÁ¦ : ÅÂ½ºÅ©¿Í ÅÂ½ºÅ©°£ÀÇ µ¿±âÈ­(¹ÂÅØ½º ÀÌ¿ë)
void kInitializeSpinLock(SPINLOCK* pstSpinLock);

void kLockForSpinLock(SPINLOCK* pstSpinLock);
void kUnlockForSpinLock(SPINLOCK* pstSpinLock);

#endif // __SYNCHRONIZATION_H__
