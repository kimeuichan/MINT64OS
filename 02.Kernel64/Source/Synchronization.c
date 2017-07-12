#include "Synchronization.h"
#include "Utility.h"
#include "Task.h"

BOOL kLockForSystemData(void){
	return kSetInterruptFlag(FALSE);
}

BOOL kUnlockForSystemData(BOOL bInterruptFlag){
	return kSetInterruptFlag(bInterruptFlag);
}

void kInitializeMutex(MUTEX* pstMutex){
	pstMutex->_LockFlag = FALSE;
	pstMutex->_LockCount = 0;
	pstMutex->_TaskID = TASK_INVALIDID;
}

void kLock(MUTEX* pstMutex){
	// 이미 잠겨 있다면 내가 잠갔는지 확인하고 잠근 횟수를 증가
	if(kTestAndSet(&(pstMutex->_LockFlag), 0, 1) == FALSE){
		if(pstMutex->_TaskID == kGetRunningTask()->stLink.qwID){
			pstMutex->_LockCount++;
			return;
		}


		while(kTestAndSet(&(pstMutex->_LockFlag), 0, 1) == FALSE){
			kSchedule();
		}
	}
	pstMutex->_LockCount = 1;
	pstMutex->_TaskID = kGetRunningTask()->stLink.qwID;
}

void kUnlock(MUTEX* pstMutex){
	if( (pstMutex->_LockFlag == FALSE) || 
		(pstMutex->_TaskID != kGetRunningTask()->stLink.qwID)){
		return;
	}

	if(pstMutex->_LockCount > 1){
		pstMutex->_LockCount--;
		return;
	}

	pstMutex->_TaskID = TASK_INVALIDID;
	pstMutex->_LockCount = 0;
	pstMutex->_LockFlag = FALSE;
}
