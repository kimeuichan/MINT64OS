#ifndef __SYNCHRONIZATION_H__
#define __SYNCHRONIZATION_H__

#include "Types.h"

/***** ����ü ���� *****/
#pragma pack(push, 1)

typedef struct kMutexStruct{
	volatile QWORD qwTaskID;    // ����� ������ �½�ũ ID
	volatile DWORD dwLockCount; // ��� Ƚ��
	volatile BOOL bLockFlag;    // ��� �÷���(��� ����)
	BYTE vbPadding[3];          // �е� ����Ʈ(�ڷᱸ���� ũ�⸦ 8����Ʈ ������ ���߷��� �߰��� �ʵ�)
} MUTEX;

#pragma pack(pop)

/***** �Լ� ���� *****/
BOOL kLockForSystemData(void);                  // ���        : �½�ũ�� ���ͷ�Ʈ���� ����ȭ
void kUnlockForSystemData(BOOL bInterruptFlag); // ��� ���� : �½�ũ�� ���ͷ�Ʈ���� ����ȭ
void kInitializeMutex(MUTEX* pstMutex);         // ���ؽ� �ʱ�ȭ
void kLock(MUTEX* pstMutex);                    // ���        : �½�ũ�� �½�ũ���� ����ȭ(���ؽ� �̿�)
void kUnlock(MUTEX* pstMutex);                  // ��� ���� : �½�ũ�� �½�ũ���� ����ȭ(���ؽ� �̿�)

#endif // __SYNCHRONIZATION_H__
