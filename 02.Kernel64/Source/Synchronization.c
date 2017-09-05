#include "Synchronization.h"
#include "Utility.h"
#include "Task.h"
#include "AssemblyUtility.h"

BOOL kLockForSystemData(void){
	return kSetInterruptFlag(FALSE);
}

void kUnlockForSystemData(BOOL bInterruptFlag){
	kSetInterruptFlag(bInterruptFlag);
}

void kInitializeMutex(MUTEX* pstMutex){
	pstMutex->bLockFlag = FALSE;
	pstMutex->dwLockCount = 0;
	pstMutex->qwTaskID = TASK_INVALIDID;
}
void kLock(MUTEX* pstMutex){

	// �̹� ��� ������ ���, �ڽ�(���� �������� �½�ũ)�� ��ɴ��� Ȯ���ϰ�, �׷��ٸ� ��� Ƚ���� ������Ű�� ����
	if(kTestAndSet(&(pstMutex->bLockFlag), FALSE, TRUE) == FALSE){
		// �ڽ��� ����� ���, ��� Ƚ���� ������Ű�� ����
		if(pstMutex->qwTaskID == kGetRunningTask()->stLink.qwID){
			pstMutex->dwLockCount++;
			return;
		}

		// �ٸ� �½�ũ�� ����� ���, ��� ���� ���°� �� ������ ���
		while(kTestAndSet(&(pstMutex->bLockFlag), FALSE, TRUE) == FALSE){
			kSchedule(); // ����ϴ� ���� ���μ����� �ٸ� �½�ũ���� �纸�����ν�, ���μ����� ���ʿ��ϰ� �Ҹ�(���)�ϴ� ���� ����
		}
	}

	// ��� ���� ������ ���, ��� ó��(pstMutex->bLockFlag = TRUE ������ kTestAndSet���� ������ �������� ó��)
	pstMutex->dwLockCount = 1;
	pstMutex->qwTaskID = kGetRunningTask()->stLink.qwID;
}
void kUnlock(MUTEX* pstMutex){

	// �̹� ��� ���� ������ ���, �Ǵ� �ٸ� �½�ũ�� ����� ���, ����
	if((pstMutex->bLockFlag == FALSE) || (pstMutex->qwTaskID != kGetRunningTask()->stLink.qwID)){
		return;
	}

	// �ߺ� ��� ������ ���, ��� Ƚ���� ���ҽ�Ű�� ����
	if(pstMutex->dwLockCount > 1){
		pstMutex->dwLockCount--;
		return;
	}

	// ���� ��� ������ ���, ��� ���� ó�� [����: ��� �÷��׸� ���� ���߿� �����ؾ� ��]
	pstMutex->qwTaskID = TASK_INVALIDID;
	pstMutex->dwLockCount = 0;
	pstMutex->bLockFlag = FALSE;
}
