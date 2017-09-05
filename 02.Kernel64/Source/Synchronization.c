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

	// 이미 잠금 상태인 경우, 자신(현재 수행중인 태스크)이 잠궜는지 확인하고, 그렇다면 잠금 횟수를 증가시키고 종료
	if(kTestAndSet(&(pstMutex->bLockFlag), FALSE, TRUE) == FALSE){
		// 자신이 잠궜을 경우, 잠금 횟수를 증가시키고 종료
		if(pstMutex->qwTaskID == kGetRunningTask()->stLink.qwID){
			pstMutex->dwLockCount++;
			return;
		}

		// 다른 태스크가 잠궜을 경우, 잠금 해제 상태가 될 때까지 대기
		while(kTestAndSet(&(pstMutex->bLockFlag), FALSE, TRUE) == FALSE){
			kSchedule(); // 대기하는 동안 프로세서를 다른 태스크에게 양보함으로써, 프로세서를 불필요하게 소모(사용)하는 것을 방지
		}
	}

	// 잠금 해제 상태인 경우, 잠금 처리(pstMutex->bLockFlag = TRUE 설정은 kTestAndSet에서 원자적 연산으로 처리)
	pstMutex->dwLockCount = 1;
	pstMutex->qwTaskID = kGetRunningTask()->stLink.qwID;
}
void kUnlock(MUTEX* pstMutex){

	// 이미 잠금 해제 상태인 경우, 또는 다른 태스크가 잠궜을 경우, 종료
	if((pstMutex->bLockFlag == FALSE) || (pstMutex->qwTaskID != kGetRunningTask()->stLink.qwID)){
		return;
	}

	// 중복 잠금 상태인 경우, 잠금 횟수만 감소시키고 종료
	if(pstMutex->dwLockCount > 1){
		pstMutex->dwLockCount--;
		return;
	}

	// 단일 잠금 상태인 경우, 잠금 해제 처리 [주의: 잠금 플래그를 가장 나중에 해제해야 함]
	pstMutex->qwTaskID = TASK_INVALIDID;
	pstMutex->dwLockCount = 0;
	pstMutex->bLockFlag = FALSE;
}
