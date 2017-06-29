#include "Task.h"
#include "Descriptor.h"
#include "Utility.h"
#include "AssemblyUtility.h"

/***** 전역 변수 정의 *****/
static SCHEDULER gs_stScheduler;
static TCBPOOLMANAGER gs_stTCBPoolManager;

//====================================================================================================
// 태스크 관련 함수
//====================================================================================================
void kInitializeTCBPool(void){
	int i;

	// TCB 풀 매니저 초기화
	kMemSet(&gs_stTCBPoolManager, 0, sizeof(gs_stTCBPoolManager));

	// TCB 풀 영역 초기화
	gs_stTCBPoolManager.pstStartAddress = (TCB*)TASK_TCBPOOLADDRESS;
	kMemSet((void*)TASK_TCBPOOLADDRESS, 0, sizeof(TCB) * TASK_MAXCOUNT);

	// TCB에 ID 할당
	for(i = 0; i < TASK_MAXCOUNT; i++){
		gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;
	}

	// TCB의 최대 개수와 할당 횟수 초기화
	gs_stTCBPoolManager.iMaxCount = TASK_MAXCOUNT;
	gs_stTCBPoolManager.iAllocatedCount = 1;
}

TCB* kAllocateTCB(void){
	TCB* pstEmptyTCB;
	int i;

	if(gs_stTCBPoolManager.iUseCount == gs_stTCBPoolManager.iMaxCount){
		return NULL;
	}

	for(i = 0; i < gs_stTCBPoolManager.iMaxCount; i++){
		// TCB ID(64비트)의 상위 32비트가 0이면 할당되지 않은 TCB
		if((gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID >> 32) == 0){
			pstEmptyTCB = &(gs_stTCBPoolManager.pstStartAddress[i]);
			break;
		}
	}

	// 할당할 TCB ID의 상위 32비트를 0이 아닌 값으로 설정 (TCB ID -> 상위 32비트:TCB 할당 횟수, 하위 32비트: TCB 일련 번호)
	pstEmptyTCB->stLink.qwID = ((QWORD)gs_stTCBPoolManager.iAllocatedCount << 32) | i;
	gs_stTCBPoolManager.iUseCount++;
	gs_stTCBPoolManager.iAllocatedCount++;

	if(gs_stTCBPoolManager.iAllocatedCount == 0){
		gs_stTCBPoolManager.iAllocatedCount = 1;
	}

	return pstEmptyTCB;
}

void kFreeTCB(QWORD qwID){
	int i;

	// TCB ID의 하위 32비트(일련 번호) 추출
	i = qwID & 0xFFFFFFFF;

	// 콘텍스트와 TCB 할당 횟수를 0으로 초기화
	kMemSet(&(gs_stTCBPoolManager.pstStartAddress[i].stContext), 0, sizeof(CONTEXT));
	gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;

	gs_stTCBPoolManager.iUseCount--;
}

TCB* kCreateTask(QWORD qwFlags, QWORD qwEntryPointAddress){
	TCB* pstTask;
	void* pvStackAddress;

	// 태스크 할당
	pstTask = kAllocateTCB();

	if(pstTask == NULL){
		return NULL;
	}

	// 태스크의 스택 어드레스 설정 (TCB ID의 일련 번호를 스택 풀의 오프셋으로 이용)
	pvStackAddress = (void*)(TASK_STACKPOOLADDRESS + (TASK_STACKSIZE * (pstTask->stLink.qwID & 0xFFFFFFFF)));

	// 태스크 설정
	kSetUpTask(pstTask, qwFlags, qwEntryPointAddress, pvStackAddress, TASK_STACKSIZE);

	// 태스크를 준비 리스트에 추가하여, 스케줄링될 수 있도록 함
	kAddTaskToReadyList(pstTask);

	return pstTask;
}

void kSetUpTask(TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress, void* pvStackAddress, QWORD qwStackSize){

	// 콘텍스트 초기화
	kMemSet(pstTCB->stContext.vqwRegister, 0, sizeof(pstTCB->stContext.vqwRegister));

	// RSP, RBP 설정
	pstTCB->stContext.vqwRegister[TASK_RSP_OFFSET] = (QWORD)pvStackAddress + qwStackSize;
	pstTCB->stContext.vqwRegister[TASK_RBP_OFFSET] = (QWORD)pvStackAddress + qwStackSize;

	// 세그먼트 셀렉터 설정
	pstTCB->stContext.vqwRegister[TASK_CS_OFFSET] = GDT_KERNELCODESEGMENT;
	pstTCB->stContext.vqwRegister[TASK_DS_OFFSET] = GDT_KERNELDATASEGMENT;
	pstTCB->stContext.vqwRegister[TASK_ES_OFFSET] = GDT_KERNELDATASEGMENT;
	pstTCB->stContext.vqwRegister[TASK_FS_OFFSET] = GDT_KERNELDATASEGMENT;
	pstTCB->stContext.vqwRegister[TASK_GS_OFFSET] = GDT_KERNELDATASEGMENT;
	pstTCB->stContext.vqwRegister[TASK_SS_OFFSET] = GDT_KERNELDATASEGMENT;

	// RIP, RFLAGS 설정
	pstTCB->stContext.vqwRegister[TASK_RIP_OFFSET] = qwEntryPointAddress;
	pstTCB->stContext.vqwRegister[TASK_RFLAGS_OFFSET] |= 0x0200; // RFLAGS의 IF(비트 9)=1로 설정하여, 인터럽트 활성화

	// 기타 설정
	pstTCB->qwFlags = qwFlags;
	pstTCB->pvStackAddress = pvStackAddress;
	pstTCB->qwStackSize = qwStackSize;
}

//====================================================================================================
// 스케줄러 관련 함수
//====================================================================================================
void kInitializeScheduler(void){

	// TCB 풀 초기화
	kInitializeTCBPool();

	// 준비 리스트 초기화
	kInitializeList(&(gs_stScheduler.stReadyList));

	// TCB를 할당받아 실행중인 태스크로 설정하여, 부팅을 수행한 태스크를 저장할 TCB를 준비
	gs_stScheduler.pstRunningTask = kAllocateTCB();
}

void kSetRunningTask(TCB* pstTask){
	gs_stScheduler.pstRunningTask = pstTask;
}

TCB* kGetRunningTask(void){
	return gs_stScheduler.pstRunningTask;
}

TCB* kGetNextTaskToRun(void){
	if(kGetListCount(&(gs_stScheduler.stReadyList)) == 0){
		return NULL;
	}

	return (TCB*)kRemoveListFromHead(&(gs_stScheduler.stReadyList));
}

void kAddTaskToReadyList(TCB* pstTask){
	kAddListToTail(&(gs_stScheduler.stReadyList), pstTask);
}

void kSchedule(void){
	TCB* pstRunningTask, * pstNextTask;
	BOOL bPreviousFlags;

	if(kGetListCount(&(gs_stScheduler.stReadyList)) == 0){
		return;
	}

	bPreviousFlags = kSetInterruptFlag(FALSE);

	pstNextTask = kGetNextTaskToRun();
	if(pstNextTask == NULL){
		kSetInterruptFlag(bPreviousFlags);
		return;
	}

	pstRunningTask = gs_stScheduler.pstRunningTask;

	// 태스크 전환(태스크 수행시)
	// - 콘텍스트 저장 : 레지스터->런닝 태스크의 콘텍스트 메모리 (kSwitchContext가 처리)
	// - 콘텍스트 복원 : 넥스트 태스크의 콘텍스트 메모리->레지스터 (kSwitchContext가 처리)
	kAddTaskToReadyList(pstRunningTask);
	gs_stScheduler.pstRunningTask = pstNextTask;
	kSwitchContext(&(pstRunningTask->stContext), &(pstNextTask->stContext));

	// 프로세서 사용시간 갱신
	gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;

	kSetInterruptFlag(bPreviousFlags);
}

BOOL kScheduleInInterrupt(void){
	TCB* pstRunningTask, * pstNextTask;
	char* pcContextAddress;

	pstNextTask = kGetNextTaskToRun();
	if(pstNextTask == NULL){
		return FALSE;
	}

	// 태스크 전환(인터럽트 핸들러 수행시)
	// - 콘텍스트 저장 : 레지스터->IST의 콘텍스트 메모리(프로세서, ISR가 처리)
	//             IST의 콘텍스트 메모리->런닝 태스크의 콘텍스트 메모리(kMemCpy가 처리)
	// - 콘텍스트 복원 : 넥스트 태스크의 콘텍스트 메모리->IST의 콘텍스트 메모리(kMemCpy가 처리)
	//             IST의 콘텍스트 메모리->레지스터(프로세서, ISR가 처리)
	pcContextAddress = (char*)IST_STARTADDRESS + IST_SIZE - sizeof(CONTEXT);
	pstRunningTask = gs_stScheduler.pstRunningTask;
	kMemCpy(&(pstRunningTask->stContext), pcContextAddress, sizeof(CONTEXT));

	kAddTaskToReadyList(pstRunningTask);
	gs_stScheduler.pstRunningTask = pstNextTask;
	kMemCpy(pcContextAddress, &(pstNextTask->stContext), sizeof(CONTEXT));

	// 프로세서 사용시간 갱신
	gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;

	return TRUE;
}

void kDecreaseProcessorTime(void){
	if(gs_stScheduler.iProcessorTime > 0){
		gs_stScheduler.iProcessorTime--;
	}
}

BOOL kIsProcessorTimeExpired(void){
	if(gs_stScheduler.iProcessorTime <= 0){
		return TRUE;
	}

	return FALSE;
}