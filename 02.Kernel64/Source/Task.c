#include "Task.h"
#include "Descriptor.h"
#include "Utility.h"
#include "AssemblyUtility.h"
#include "Synchronization.h"
#include "Console.h"

/***** 전역 변수 정의 *****/
static SCHEDULER gs_stScheduler;
static TCBPOOLMANAGER gs_stTCBPoolManager;

//====================================================================================================
// 태스크 관련 함수
//====================================================================================================
static void kInitializeTCBPool(void){
	int i;

	// TCB 풀 매니저 초기화
	kMemSet(&gs_stTCBPoolManager, 0, sizeof(gs_stTCBPoolManager));

	// TCB 풀 영역 초기화
	gs_stTCBPoolManager.pstStartAddress = (TCB*)TASK_TCBPOOLADDRESS;
	kMemSet((void*)TASK_TCBPOOLADDRESS, 0, sizeof(TCB) * TASK_MAXCOUNT);

	// TCB에 ID(오프셋) 할당
	for(i = 0; i < TASK_MAXCOUNT; i++){
		gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;
	}

	// TCB의 최대 개수와 할당 횟수 초기화
	gs_stTCBPoolManager.iMaxCount = TASK_MAXCOUNT;
	gs_stTCBPoolManager.iAllocatedCount = 1;
}

static TCB* kAllocateTCB(void){
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

	// 할당할 TCB ID의 상위 32비트를 0이 아닌 값으로 설정 (TCB ID -> 상위 32비트:TCB 할당 횟수, 하위 32비트: TCB 오프셋)
	pstEmptyTCB->stLink.qwID = ((QWORD)gs_stTCBPoolManager.iAllocatedCount << 32) | i;
	gs_stTCBPoolManager.iUseCount++;
	gs_stTCBPoolManager.iAllocatedCount++;

	if(gs_stTCBPoolManager.iAllocatedCount == 0){
		gs_stTCBPoolManager.iAllocatedCount = 1;
	}

	return pstEmptyTCB;
}

static void kFreeTCB(QWORD qwID){
	int i;

	// TCB ID의 하위 32비트(TCB 오프셋) 추출
	i = GETTCBOFFSET(qwID);

	// 콘텍스트와 TCB 할당 횟수를 0으로 초기화
	kMemSet(&(gs_stTCBPoolManager.pstStartAddress[i].stContext), 0, sizeof(CONTEXT));
	gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;

	gs_stTCBPoolManager.iUseCount--;
}

TCB* kCreateTask(QWORD qwFlags, void* pvMemoryAddress, QWORD qwMemorySize, QWORD qwEntryPointAddress){
	TCB* pstTask;    // 생성할 태스크(프로세스 또는 스레드)
	TCB* pstProcess; // 현재 태스크가 속한 프로세스(자신을 생성한 프로세스, 부모 프로세스)
	void* pvStackAddress;
	BOOL bPreviousFlag;

	bPreviousFlag = kLockForSystemData();

	// 태스크 할당
	pstTask = kAllocateTCB();

	if(pstTask == NULL){
		kUnlockForSystemData(bPreviousFlag);
		return NULL;
	}

	// 현재 태스크가 속한 프로세스(자신을 생성한 프로세스, 부모 프로세스) 취득
	pstProcess = kGetProcessByThread(kGetRunningTask());

	if(pstProcess == NULL){
		kFreeTCB(pstTask->stLink.qwID);
		kUnlockForSystemData(bPreviousFlag);
		return NULL;
	}

	//****************************************************************************************************
	// 태스크 관리 리스트의 도식화
	// 1. 태스크 생성시
	//   -------------------
	//   |        -------- |  -> 바깥쪽 사각형은 준비 리스트(Ready List)
	//   |P1->P2->|T3=>T4| |  -> 안쪽 사각형은 자식 스레드 리스트(Child Thread List)
	//   |        -------- |
	//   -------------------
	// 2. P2 프로세스 종료시
	//   ------------
	//   |T3->T4->P2|  -> 대기 리스트(Wait List)
	//   ------------
	// 3. T3 스레드 종료시
	//   ----
	//   |T3|  -> 대기 리스트(Wait List)
	//   ----
	//****************************************************************************************************

	// 스레드를 생성하는 경우
	if(qwFlags & TASK_FLAGS_THREAD){

		// [스레드.부모 프로세스 ID]에 자신을 생성한 프로세스(부모 프로세스)의 ID를 설정하고, [스레드.메모리 영역]에 자신을 생성한 프로세스(부모 프로세스)의 메모리 역역을 설정함
		pstTask->qwParentProcessID = pstProcess->stLink.qwID;
		pstTask->pvMemoryAddress = pstProcess->pvMemoryAddress;
		pstTask->qwMemorySize = pstProcess->qwMemorySize;

		// 자신을 생성한 프로세스(부모 프로세스)의 자식 스레드 리스트에 생성한 스레드를 추가
		kAddListToTail(&(pstProcess->stChildThreadList), &(pstTask->stThreadLink));

	// 프로세스를 생성하는 경우
	}else{
		// [프로세스.부모 프로세스 ID]에 자신을 생성한 프로세스(부모 프로세스)의 ID를 설정하고, [프로세스.메모리 영역]에 파라미터로 넘어 온 메모리 영역을 설정함
		pstTask->qwParentProcessID = pstProcess->stLink.qwID;
		pstTask->pvMemoryAddress = pvMemoryAddress;
		pstTask->qwMemorySize = qwMemorySize;
	}

	// 스레드 ID를 태스크 ID와 동일하게 설정
	pstTask->stThreadLink.qwID = pstTask->stLink.qwID;

	kUnlockForSystemData(bPreviousFlag);

	// 태스크의 스택 어드레스 설정 (TCB ID의 오프셋를 스택 풀의 오프셋으로 이용)
	pvStackAddress = (void*)(TASK_STACKPOOLADDRESS + (TASK_STACKSIZE * GETTCBOFFSET(pstTask->stLink.qwID)));

	// 태스크 설정
	kSetUpTask(pstTask, qwFlags, qwEntryPointAddress, pvStackAddress, TASK_STACKSIZE);

	// 자식 스레드 리스트 초기화
	kInitializeList(&(pstTask->stChildThreadList));

	// FPU 사용 여부 초기화
	pstTask->bFPUUsed = FALSE;

	bPreviousFlag = kLockForSystemData();

	// 태스크를 준비 리스트에 추가하여, 스케줄링될 수 있도록 함
	kAddTaskToReadyList(pstTask);

	kUnlockForSystemData(bPreviousFlag);

	return pstTask;
}

static void kSetUpTask(TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress, void* pvStackAddress, QWORD qwStackSize){

	// 콘텍스트 초기화
	kMemSet(pstTCB->stContext.vqwRegister, 0, sizeof(pstTCB->stContext.vqwRegister));

	// RSP, RBP 설정
	pstTCB->stContext.vqwRegister[TASK_RSP_OFFSET] = (QWORD)pvStackAddress + qwStackSize - 8;
	pstTCB->stContext.vqwRegister[TASK_RBP_OFFSET] = (QWORD)pvStackAddress + qwStackSize - 8;

	// 스택의 최상위 8바이트 영역에 복귀 주소로서  kExitTask()함수의 어드레스를 삽입함으로써,
	// 태스크의 엔트리 포인트 함수가 return 되는 순간, kExitTask()함수로 이동하게 함
	*(QWORD*)((QWORD)pvStackAddress + qwStackSize - 8) = (QWORD)kExitTask;

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
	pstTCB->pvStackAddress = pvStackAddress;
	pstTCB->qwStackSize = qwStackSize;
	pstTCB->qwFlags = qwFlags;
}

//====================================================================================================
// 스케줄러 관련 함수
//====================================================================================================
void kInitializeScheduler(void){
	int i;
	TCB* pstTask;

	// TCB 풀 초기화
	kInitializeTCBPool();

	for(i = 0; i < TASK_MAXREADYLISTCOUNT; i++){
		// 준비 리스트 초기화
		kInitializeList(&(gs_stScheduler.vstReadyList[i]));

		// 우선순위별 태스크 실행 횟수 초기화
		gs_stScheduler.viExecuteCount[i] = 0;
	}

	// 대기 리스트 초기화
	kInitializeList(&(gs_stScheduler.stWaitList));

	// TCB를 할당받아 실행중인 태스크로 설정하여, 부팅을 수행한 태스크(의 콘텍스트)를 저장할 TCB를 준비
	// (부팅을 수행한 태스크가 커널 최초 프로세스가 됨)
	pstTask = kAllocateTCB();
	gs_stScheduler.pstRunningTask = pstTask;
	pstTask->qwFlags = TASK_FLAGS_HIGHEST | TASK_FLAGS_PROCESS | TASK_FLAGS_SYSTEM; // 부팅을 수행한 태스크(이후에  콘솔 쉘 태스크가 됨)의 우선순위를 가장 높음으로 설정
	pstTask->qwParentProcessID = pstTask->stLink.qwID; // 커널 최초 프로세스는 부모 프로세스가 존재 하지 않음으로, 부모 프로세스 ID에 자신의 ID를 설정
	pstTask->pvMemoryAddress = (void*)0x100000;
	pstTask->qwMemorySize = 0x500000;
	pstTask->pvStackAddress = (void*)0x600000;
	pstTask->qwStackSize = 0x100000;

	// 프로세서 사용률 관련 변수 초기화
	gs_stScheduler.qwSpendProcessorTimeInIdleTask = 0;
	gs_stScheduler.qwProcessorLoad = 0;

	// 마지막 FPU 사용 태스크 ID 초기화
	gs_stScheduler.qwLastFPUUsedTaskID = TASK_INVALIDID;
}

void kSetRunningTask(TCB* pstTask){
	BOOL bPreviousFlag;

	bPreviousFlag = kLockForSystemData();

	gs_stScheduler.pstRunningTask = pstTask;

	kUnlockForSystemData(bPreviousFlag);
}

TCB* kGetRunningTask(void){
	TCB* pstRunningTask;
	BOOL bPreviousFlag;

	bPreviousFlag = kLockForSystemData();

	pstRunningTask = gs_stScheduler.pstRunningTask;

	kUnlockForSystemData(bPreviousFlag);

	return pstRunningTask;
}

static TCB* kGetNextTaskToRun(void){
	TCB* pstTarget = NULL;
	int iTaskCount;
	int i, j;

	// 모든 준비 리스트의 태스크가 1회씩 실행된 경우, 모든 준비 리스트가 프로세서를 양보하여
	// 준비 리스트에 태스크가 있음에도 불구하고, 태스크를 선택하지 못 할 수 있으니
	// 태스크가 NULL일 경우, 한번 더 수행
	for(j = 0; j < 2; j++){
		// 높은 우선순위에서 낮은 우선순위까지 준비 리스트를 확인하여, 다음에 실행할 태스크를 선택
		for(i = 0; i < TASK_MAXREADYLISTCOUNT; i++){
			// 우선순위별 태스크 개수 취득
			iTaskCount = kGetListCount(&(gs_stScheduler.vstReadyList[i]));

			// 해당 준비 리스트의 태스크 실행 횟수보다 태스크 개수가 많은 경우, 현재 우선순위의 태스크를 선택
			if(gs_stScheduler.viExecuteCount[i] < iTaskCount){
				pstTarget = (TCB*)kRemoveListFromHead(&(gs_stScheduler.vstReadyList[i]));
				gs_stScheduler.viExecuteCount[i]++;
				break;

			// 해당 준비 리스트의 태스크 실행 횟수가 태스크 개수와 같거나 많은 경우, 다음 우선순위의 태스크에 양보
			}else{
				gs_stScheduler.viExecuteCount[i] = 0;
			}

		}

		// 다음에 실행할 태스크를 선택한 경우, 루프 탈출
		if(pstTarget != NULL){
			break;
		}
	}

	return pstTarget;
}

static BOOL kAddTaskToReadyList(TCB* pstTask){
	BYTE bPriority;

	bPriority = GETPRIORITY(pstTask->qwFlags);

	if(bPriority >= TASK_MAXREADYLISTCOUNT){
		return FALSE;
	}

	kAddListToTail(&(gs_stScheduler.vstReadyList[bPriority]), pstTask);
	return TRUE;
}

static TCB* kRemoveTaskFromReadyList(QWORD qwTaskID){
	TCB* pstTarget;
	BYTE bPriority;

	// 태스크 ID가 유효한지 확인
	if(GETTCBOFFSET(qwTaskID) >= TASK_MAXCOUNT){
		return NULL;
	}

	// 태스크 풀에서 해당 태스크를 찾아 실제로 ID가 일치하는지 확인
	pstTarget = &(gs_stTCBPoolManager.pstStartAddress[GETTCBOFFSET(qwTaskID)]);
	if(pstTarget->stLink.qwID != qwTaskID){
		return NULL;
	}

	bPriority = GETPRIORITY(pstTarget->qwFlags);

	// 해당 우선순위의 준비 리스트에서 태스크 삭제
	pstTarget = kRemoveList(&(gs_stScheduler.vstReadyList[bPriority]), qwTaskID);

	return pstTarget;
}

BOOL kChangePriority(QWORD qwTaskID, BYTE bPriority){
	TCB* pstTarget;
	BOOL bPreviousFlag;

	if(bPriority >= TASK_MAXREADYLISTCOUNT){
		return FALSE;
	}

	bPreviousFlag = kLockForSystemData();

	pstTarget = gs_stScheduler.pstRunningTask;

	// 현재 실행중인 태스크인 경우, 우선순위만 변경
	// 타이머 인터럽트(IRQ0)가 발생하여 태스크 전환이 수행될 때, 변경된 우선순위의 준비리스트로 이동됨
	if(pstTarget->stLink.qwID == qwTaskID){
		SETPRIORITY(pstTarget->qwFlags, bPriority);

    // 현재 실행중인 태스크가 아닌 경우, 준비 리스트에서 삭제하고, 우선순위를 변경한 후, 변경된 우선순위의 준비리스트로 이동시킴
	}else{
		pstTarget = kRemoveTaskFromReadyList(qwTaskID);

		// 준비 리스트에 없는 경우, 태스크 풀에서 찾아서 직접 우선순위 변경
		if(pstTarget == NULL){
			pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
			if(pstTarget != NULL){
				SETPRIORITY(pstTarget->qwFlags, bPriority);
			}
		// 준비 리스트에 있는 경우, 우선순위를 변경한 후, 변경된 우선순위의 준비리스트로 이동시킴
		}else{
			SETPRIORITY(pstTarget->qwFlags, bPriority);
			kAddTaskToReadyList(pstTarget);
		}
	}

	kUnlockForSystemData(bPreviousFlag);

	return TRUE;
}

void kSchedule(void){
	TCB* pstRunningTask, * pstNextTask;
	BOOL bPreviousFlag;

	if(kGetReadyTaskCount() < 1){
		return;
	}

	bPreviousFlag = kLockForSystemData();

	pstNextTask = kGetNextTaskToRun();
	if(pstNextTask == NULL){
		kUnlockForSystemData(bPreviousFlag);
		return;
	}

	//****************************************************************************************************
	// 태스크 전환(태스크 수행시)
	// - 콘텍스트 저장 : 레지스터->런닝 태스크의 콘텍스트 메모리 (kSwitchContext가 처리)
	// - 콘텍스트 복원 : 넥스트 태스크의 콘텍스트 메모리->레지스터 (kSwitchContext가 처리)
	//****************************************************************************************************

	pstRunningTask = gs_stScheduler.pstRunningTask;
	gs_stScheduler.pstRunningTask = pstNextTask;

	// 유휴 태스크에서 전환된 경우, 유휴 태스크가 사용한 프로세서 시간을 증가시킴
	if((pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE){
		gs_stScheduler.qwSpendProcessorTimeInIdleTask += (TASK_PROCESSORTIME - gs_stScheduler.iProcessorTime);
	}

	// 다음 태스크가 마지막 FPU 사용 태스크가 아닌 경우, CR0.TS=1 로 설정
	if(gs_stScheduler.qwLastFPUUsedTaskID != pstNextTask->stLink.qwID){
		kSetTS();

	}else{
		kClearTS();
	}

	// 종료 태스크에서 전환된 경우, 종료 태스크를 대기 리스트로 이동시키고, 태스크 전환 수행
	if((pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK) == TASK_FLAGS_ENDTASK){
		kAddListToTail(&(gs_stScheduler.stWaitList), pstRunningTask);
		kSwitchContext(NULL, &(pstNextTask->stContext));

	// 일반 태스크에서 전환된 경우, 일반 태스크를 준비 리스트로 이동시키고, 태스크 전환 수행
	}else{
		kAddTaskToReadyList(pstRunningTask);
		kSwitchContext(&(pstRunningTask->stContext), &(pstNextTask->stContext));
	}

	// 프로세서 사용시간 갱신
	gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;

	kUnlockForSystemData(bPreviousFlag);
}

BOOL kScheduleInInterrupt(void){
	TCB* pstRunningTask, * pstNextTask;
	char* pcContextAddress;
	BOOL bPreviousFlag;

	bPreviousFlag = kLockForSystemData();

	pstNextTask = kGetNextTaskToRun();
	if(pstNextTask == NULL){
		kUnlockForSystemData(bPreviousFlag);
		return FALSE;
	}

	//****************************************************************************************************
	// 태스크 전환(인터럽트 핸들러 수행시)
	// - 콘텍스트 저장 : 레지스터->IST의 콘텍스트 메모리(프로세서, ISR가 처리)
	//             IST의 콘텍스트 메모리->런닝 태스크의 콘텍스트 메모리(kMemCpy가 처리)
	// - 콘텍스트 복원 : 넥스트 태스크의 콘텍스트 메모리->IST의 콘텍스트 메모리(kMemCpy가 처리)
	//             IST의 콘텍스트 메모리->레지스터(프로세서, ISR가 처리)
	//****************************************************************************************************

	pcContextAddress = (char*)IST_STARTADDRESS + IST_SIZE - sizeof(CONTEXT);
	pstRunningTask = gs_stScheduler.pstRunningTask;
	gs_stScheduler.pstRunningTask = pstNextTask;

	// 유휴 태스크에서 전환된 경우, 유휴 태스크가 사용한 프로세서 시간을 증가시킴
	if((pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE){
		gs_stScheduler.qwSpendProcessorTimeInIdleTask += TASK_PROCESSORTIME;
	}

	// 종료 태스크에서 전환된 경우, 종료 태스크를 대기 리스트로 이동시키고, 태스크 전환 수행
	if((pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK) == TASK_FLAGS_ENDTASK){
		kAddListToTail(&(gs_stScheduler.stWaitList), pstRunningTask);

	// 일반 태스크에서 전환된 경우, 일반 태스크를 준비 리스트로 이동시키고, 태스크 전환 수행
	}else{
		kMemCpy(&(pstRunningTask->stContext), pcContextAddress, sizeof(CONTEXT));
		kAddTaskToReadyList(pstRunningTask);
	}

	kUnlockForSystemData(bPreviousFlag);

	// 다음 태스크가 마지막 FPU 사용 태스크가 아닌 경우, CR0.TS=1 로 설정
	if(gs_stScheduler.qwLastFPUUsedTaskID != pstNextTask->stLink.qwID){
		kSetTS();

	}else{
		kClearTS();
	}

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

BOOL kEndTask(QWORD qwTaskID){
	TCB* pstTarget;
	BYTE bPriority;
	BOOL bPreviousFlag;

	bPreviousFlag = kLockForSystemData();

	pstTarget = gs_stScheduler.pstRunningTask;

	// 현재 실행중인 태스크인 경우, 종료 태스크 비트를 설정하고, 태스크 전환 수행
	if(pstTarget->stLink.qwID == qwTaskID){
		pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
		SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);

		kUnlockForSystemData(bPreviousFlag);

		// 태스크 전환
		kSchedule();

		// 태스크가 전환되었으므로 아래 코드는 실행되지 않음
		while(1);

	// 현재 실행중인 태스크가 아닌 경우, 준비 리스트에서 삭제하고, 종료 태스크 비트를 설정한 후, 대기 리스트로 이동시킴
	}else{
		pstTarget = kRemoveTaskFromReadyList(qwTaskID);

		if(pstTarget == NULL){
			pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
			if(pstTarget != NULL){
				pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
				SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
			}

			kUnlockForSystemData(bPreviousFlag);

			return TRUE;
		}

		pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
		SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
		kAddListToTail(&(gs_stScheduler.stWaitList), pstTarget);
	}

	kUnlockForSystemData(bPreviousFlag);

	return TRUE;
}

void kExitTask(void){
	kEndTask(gs_stScheduler.pstRunningTask->stLink.qwID);
}

int kGetReadyTaskCount(void){
	int iTotalCount = 0;
	int i;
	BOOL bPreviousFlag;

	bPreviousFlag = kLockForSystemData();

	for(i = 0; i < TASK_MAXREADYLISTCOUNT; i++){
		iTotalCount += kGetListCount(&(gs_stScheduler.vstReadyList[i]));
	}

	kUnlockForSystemData(bPreviousFlag);

	return iTotalCount;
}

int kGetTaskCount(void){
	int iTotalCount;
	BOOL bPreviousFlag;

	//총 태스크 수 = 실행 준비 태스크 수 + 종료 대기 태스크 수 + 현재 실행중인 태스크 수
	iTotalCount = kGetReadyTaskCount();

	bPreviousFlag = kLockForSystemData();

	iTotalCount += kGetListCount(&(gs_stScheduler.stWaitList)) + 1;

	kUnlockForSystemData(bPreviousFlag);

	return iTotalCount;
}

TCB* kGetTCBInTCBPool(int iOffset){
	if((iOffset <= -1) && (iOffset >= TASK_MAXCOUNT)){
		return NULL;
	}

	return &(gs_stTCBPoolManager.pstStartAddress[iOffset]);
}

BOOL kIsTaskExist(QWORD qwTaskID){
	TCB* pstTCB;

	pstTCB = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));

	if((pstTCB == NULL) || (pstTCB->stLink.qwID != qwTaskID)){
		return FALSE;
	}

	return TRUE;
}

QWORD kGetProcessorLoad(void){
	return gs_stScheduler.qwProcessorLoad;
}

static TCB* kGetProcessByThread(TCB* pstThread){
	TCB* pstProcess;

	// 파라미터가 프로세스인 경우, 자신을 리턴
	if(pstThread->qwFlags & TASK_FLAGS_PROCESS){
		return pstThread;
	}

	// 파라미터가 스레드인 경우, 부모 프로세스를 리턴
	pstProcess = kGetTCBInTCBPool(GETTCBOFFSET(pstThread->qwParentProcessID));
	if((pstProcess == NULL) || (pstProcess->stLink.qwID != pstThread->qwParentProcessID)){
		return NULL;
	}

	return pstProcess;
}

//====================================================================================================
// 유휴 태스크 관련 함수
//====================================================================================================
void kIdleTask(void){
	TCB* pstTask, * pstChildThread, * pstProcess;
	QWORD qwLastMeasureTickCount, qwLastSpendTickInIdleTask;
	QWORD qwCurrentMeasureTickCount, qwCurrentSpendTickInIdleTask;
	BOOL bPreviousFlag;
	int i, iCount;
	QWORD qwTaskID;
	void* pvThreadLink;

	qwLastMeasureTickCount = kGetTickCount();
	qwLastSpendTickInIdleTask = gs_stScheduler.qwSpendProcessorTimeInIdleTask;

	// 무한 루프
	while(1){

		//----------------------------------------------------------------------------------------------------
		// 1. 프로세서 사용률 계산
		//----------------------------------------------------------------------------------------------------

		qwCurrentMeasureTickCount = kGetTickCount();
		qwCurrentSpendTickInIdleTask = gs_stScheduler.qwSpendProcessorTimeInIdleTask;

		// 프로세서 사용률(%) = 100 - (유휴 태스크가 사용한 프로세서 시간 * 100 / 시스템 전체에서 사용한 프로세서 시간)
		if((qwCurrentMeasureTickCount - qwLastMeasureTickCount) == 0){
			gs_stScheduler.qwProcessorLoad = 0;

		}else{
			gs_stScheduler.qwProcessorLoad = 100 - ((qwCurrentSpendTickInIdleTask - qwLastSpendTickInIdleTask) * 100 / (qwCurrentMeasureTickCount - qwLastMeasureTickCount));
		}

		qwLastMeasureTickCount = qwCurrentMeasureTickCount;
		qwLastSpendTickInIdleTask = qwCurrentSpendTickInIdleTask;

		//----------------------------------------------------------------------------------------------------
		// 2. 프로세서 사용률에 따라 프로세서를 쉬게 함
		//----------------------------------------------------------------------------------------------------

		// 프로세서 부하(프로세서 사용률)에 따라 프로세서를 쉬게 함
		kHaltProcessorByLoad();

		//----------------------------------------------------------------------------------------------------
		// 3. 대기 리스트의 태스크를 종료시킴
		//----------------------------------------------------------------------------------------------------

		// 대기 리스트에 종료 태스크가 있는 경우, 대기 리스트에서 종료 태스크를 삭제하고, 종료 태스크의 메모리를 해제함
		if(kGetListCount(&(gs_stScheduler.stWaitList)) > 0){
			while(1){

				bPreviousFlag = kLockForSystemData();

				// 대기 리스트에서 종료 태스크를 삭제
				pstTask = kRemoveListFromHead(&(gs_stScheduler.stWaitList));
				if(pstTask == NULL){
					kUnlockForSystemData(bPreviousFlag);
					break;
				}

				// [코드 블록 1]종료 태스크가 프로세스인 경우, 프로세스의 자식 스레드를 모두 종료(준비 리스트->대기 리스트)시킨 후,
				//          우휴 태스크가 대기 리스트의 자식 스레드를 모두 완전히 종료(메모리 해제)시킬 때까지 대기 한 후 마지막에 프로세스 자신을 완전히 종료(메모리 해제)시킴
				// [참고] 프로세스의 메모리 해제 : 코드/데이터 영역, TCB 영역, 스택 영역
				//      스레드의 메모리 해제     : TCB 영역, 스택 영역
				if(pstTask->qwFlags & TASK_FLAGS_PROCESS){
					iCount = kGetListCount(&(pstTask->stChildThreadList));

					for(i = 0; i < iCount; i++){
						pvThreadLink = (TCB*)kRemoveListFromHead(&(pstTask->stChildThreadList));
						if(pvThreadLink == NULL){
							break;
						}

						pstChildThread = GETTCBFROMTHREADLINK(pvThreadLink);

						// [주의]자식 스레드를 자식 스레드 리스트에서 삭제한 후, 다시 삽입하는 이유
						//      첫번째 이유 : 스레드를 종료시키면 종료된 스레드는 [코드 블록 2]에서 자신을 스스로 자식 스레드 리스트에서 삭제함
						//      두번째 이유 : 멀티 코어 프로세서 환경일 경우, 자식 스레드가 다른 코어에서 실행 중일 수 있으며, 이런 경우 자식 스레드를 즉시 대기 리스트로 이동시킬 수 없음
						kAddListToTail(&(pstTask->stChildThreadList), &(pstChildThread->stThreadLink));

						// 자식 스레드를 모두 종료(준비 리스트->대기 리스트)시킴
						kEndTask(pstChildThread->stLink.qwID);
					}

					// 자식 스레드가 남아 있는 경우, 우휴 태스크가 대기 리스트의 자식 스레드를 모두 완전히 종료(메모리 해제)시킬 때까지 대기 함
					if(kGetListCount(&(pstTask->stChildThreadList)) > 0){
						kAddListToTail(&(gs_stScheduler.stWaitList), pstTask);

						kUnlockForSystemData(bPreviousFlag);
						continue;

					// 자식 스레드가 모두 완전히 종료(메모리 해제)된 경우, 프로세스 자신을 완전히 종료(메모리 해제)시킴
					}else{
						// TODO: 추후에 코드 삽입
						// 종료 프로세스의 코드/데이터 영역 메모리를 해제
					}

				// [코드 블록 2]종료 태스크가 스레드인 경우, 스레드가 속한 프로세스의 자식 스레드 리스트에서 자신을 삭제하고, 스레드 자신을 완전히 종료(메모리 해제)시킴
				}else if(pstTask->qwFlags & TASK_FLAGS_THREAD){
					pstProcess = kGetProcessByThread(pstTask);

					if(pstProcess != NULL){
						kRemoveList(&(pstProcess->stChildThreadList), pstTask->stLink.qwID);
					}
				}

				// 종료 태스크의 TCB 영역 메모리를 해제(TCB 영역을 해제하면, 스택 영역은 자동으로 해제됨)
				qwTaskID = pstTask->stLink.qwID;
				kFreeTCB(qwTaskID);

				kUnlockForSystemData(bPreviousFlag);

				kPrintf("IDLE: Task ID[0x%q] is completely ended.\n", qwTaskID);
			}
		}

		// 태스크 전환
		kSchedule();
	}
}

void kHaltProcessorByLoad(void){

	if(gs_stScheduler.qwProcessorLoad < 40){
		kHlt();
		kHlt();
		kHlt();

	}else if(gs_stScheduler.qwProcessorLoad < 80){
		kHlt();
		kHlt();

	}else if(gs_stScheduler.qwProcessorLoad < 95){
		kHlt();
	}
}

//====================================================================================================
// FPU 관련 함수
//====================================================================================================
QWORD kGetLastFPUUsedTaskID(void){
	return gs_stScheduler.qwLastFPUUsedTaskID;
}

void kSetLastFPUUsedTaskID(QWORD qwTaskID){
	gs_stScheduler.qwLastFPUUsedTaskID = qwTaskID;
}
