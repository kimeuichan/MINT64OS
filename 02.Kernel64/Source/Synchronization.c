#include "Synchronization.h"
#include "Utility.h"
#include "Task.h"
#include "AssemblyUtility.h"

#if 0
BOOL kLockForSystemData(void){
	return kSetInterruptFlag(FALSE);
}

void kUnlockForSystemData(BOOL bInterruptFlag){
	kSetInterruptFlag(bInterruptFlag);
}
#endif

void kInitializeMutex(MUTEX* pstMutex){
	pstMutex->bLockFlag = FALSE;
	pstMutex->dwLockCount = 0;
	pstMutex->qwTaskID = TASK_INVALIDID;
}
void kLock(MUTEX* pstMutex) {
	BYTE bCurrentAPICID;
	BOOL bInterruptFlag;

	// 인터럽트를 비활성화
	bInterruptFlag = kSetInterruptFlag(FALSE);

	bCurrentAPICID = kGetAPICID();

	//bLockFlag와 두번째인자(FALSE)를 비교를 하여서, 만약 같은 값이면 첫번째 인자의 값은 세번째 인자값(TRUE)가 된다.
	//즉, 아래 코드에서는 bLockFlag가 FALSE일 경우 bLockFlag의 값을 TRUE로 바꾼 뒤, 함수는 TRUE를 리턴한다.
	//만약 TRUE일 경우에는 bLockFlag의 값이 변하지 않고 함수는 FALSE를 리턴
	if(kTestAndSet(&(pstMutex->bLockFlag),0,1)==FALSE) { //flag가 true일 경우
		//이미 잠겨져 있을 때, 동일한 ID의 태스크가 다시 한번 잠글려고 한다면, 카운트를 증가시킴
		if(pstMutex->qwTaskID==kGetRunningTask(bCurrentAPICID)->stLink.qwID) {
			kSetInterruptFlag(bInterruptFlag);
			pstMutex->dwLockCount++;
			return;
		}
		//잠금해제될 때까지 계속해서 다른 태스크에게 양보
		while(kTestAndSet(&(pstMutex->bLockFlag),0,1)==FALSE)
			kSchedule();
	}
	pstMutex->dwLockCount = 1;
	pstMutex->qwTaskID = kGetRunningTask(bCurrentAPICID)->stLink.qwID;
	kSetInterruptFlag(bInterruptFlag);
}

void kUnlock(MUTEX* pstMutex) {
	//이미 풀려있거나, 뮤택스를 잠근 태스크가 아닌 다른 태스크가 해제할려고 하면 종료

	BOOL bInterruptFlag;

	bInterruptFlag = kSetInterruptFlag(FALSE);

	if((pstMutex->bLockFlag==FALSE) || (pstMutex->qwTaskID!=kGetRunningTask(kGetAPICID())->stLink.qwID)){
		kSetInterruptFlag(bInterruptFlag);
		return;
	}

	if(pstMutex->dwLockCount>1) {
		pstMutex->dwLockCount--;
	}
	else{
		pstMutex->qwTaskID = TASK_INVALIDID;
		pstMutex->dwLockCount = 0;
		pstMutex->bLockFlag = FALSE;
	}
	
	kSetInterruptFlag(bInterruptFlag);
}


void kInitializeSpinLock(SPINLOCK* pstSpinLock){
	pstSpinLock->bLockFlag = FALSE;
	pstSpinLock->dwLockCount = 0;
	pstSpinLock->bAPICID = 0xff;
	pstSpinLock->bInterruptFlag = FALSE;
}

// 시스템 전역에서 사용하는 데이터를 위한 잠금 함수
void kLockForSpinLock(SPINLOCK* pstSpinLock){
	BOOL bInterruptFlag;

	bInterruptFlag = kSetInterruptFlag(FALSE);

	// 이미 잠겨있다면 자신이 잠갔는지 확인하고 자신이 잠근거라면 횟수를 증가한 뒤 종료
	if(kTestAndSet(&(pstSpinLock->bLockFlag), 0, 1) == FALSE){
		if(pstSpinLock->bAPICID == kGetAPICID()){
			pstSpinLock->dwLockCount++;
			return;
		}

		// 자신이 아닌 경우는 잠긴것이 해제될 때 까지 대기
		while(kTestAndSet(&(pstSpinLock->bLockFlag), 0, 1) == FALSE){
			while(pstSpinLock->bLockFlag == TRUE){
				kPause();
			}
		}
	}

	

	pstSpinLock->dwLockCount = 1;
	pstSpinLock->bAPICID = kGetAPICID();
	pstSpinLock->bInterruptFlag = bInterruptFlag;
}

void kUnlockForSpinLock(SPINLOCK* pstSpinLock){
	BOOL bInterruptFlag;

	bInterruptFlag = kSetInterruptFlag(FALSE);

	if( (pstSpinLock->bLockFlag == FALSE )|| (pstSpinLock->bAPICID != kGetAPICID())){
		kSetInterruptFlag(bInterruptFlag);
		return;
	}

	if(pstSpinLock->dwLockCount > 1){
		pstSpinLock->dwLockCount--;
		return;
	}

	bInterruptFlag = pstSpinLock->bInterruptFlag;
	pstSpinLock->bAPICID = 0xff;
	pstSpinLock->dwLockCount = 0;
	pstSpinLock->bInterruptFlag = FALSE;
	pstSpinLock->bLockFlag = FALSE;

	kSetInterruptFlag(bInterruptFlag);
}