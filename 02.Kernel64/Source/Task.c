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
static void kInitializeTCBPool(void){
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

	// 할당할 TCB ID의 상위 32비트를 0이 아닌 값으로 설정 (TCB ID -> 상위 32비트:TCB 할당 횟수, 하위 32비트: TCB 일련 번호)
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

	// TCB ID의 하위 32비트(일련 번호) 추출
	i = GETTCBOFFSET(qwID);

	// 콘텍스트와 TCB 할당 횟수를 0으로 초기화
	kMemSet(&(gs_stTCBPoolManager.pstStartAddress[i].stContext), 0, sizeof(CONTEXT));
	gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;

	gs_stTCBPoolManager.iUseCount--;
}

TCB* kCreateTask(QWORD qwFlags, void* pvMemoryAddress, QWORD qwMemorySize,
				QWORD qwEntryPointAddress){
	TCB* pstTask, *pstProcess;
	void* pvStackAddress;
	BOOL bPreviousFlags;

	// 태스크 할당
	bPreviousFlags = kLockForSystemData();
	pstTask = kAllocateTCB();

	if(pstTask == NULL){
		kUnlockForSystemData(bPreviousFlags);
		return NULL;
	}

	// 현재 프로세스 또는 스레드가 속한 프로세스를 검색
	pstProcess = kGetProcessByThread(kGetRunningTask());

	// 만약 프로세스 또는 스레드가 없다면 아무런 작업 하지 않음
	if(pstProcess == NULL){
		kFreeTCB(pstTask->stLink.qwID);
		kUnlockForSystemData(bPreviousFlags);
		return NULL;
	}

	// 스레드를 생성하는 경우라면 내가 속한 프로세스의 자식 스레드 리스트에 연결
	if(qwFlags & TASK_FLAGS_THREAD){
		// 현재 스레드의 프로세스를 찾아서 생성할 스레드에 프로세스 정보를 상속
		pstTask->qwParentProcessID = pstProcess->stLink.qwID;
		pstTask->pvMemoryAddress = pstProcess->pvMemoryAddress;
		pstTask->qwMemorySize = pstProcess->qwMemorySize;

		// 부모 프로세스의 자식 스레드 리스트에 추가
		kAddListToTail( &(pstProcess->stChildThreadList), &(pstTask->stThreadLink));
	}
	// 프로세스는 파라미터러 넘어온 값을 그대로 설정
	else{
		pstTask->qwParentProcessID = pstProcess->stLink.qwID;
		pstTask->pvMemoryAddress = pvMemoryAddress;
		pstTask->qwMemorySize = qwMemorySize;
	}

	// 스레드의 ID를 태스크 ID와 동일하게 설정
	pstTask->stThreadLink.qwID = pstTask->stLink.qwID;

	kUnlockForSystemData(bPreviousFlags);

	// 태스크의 스택 어드레스 설정 (TCB ID의 일련 번호를 스택 풀의 오프셋으로 이용)
	pvStackAddress = (void*)(TASK_STACKPOOLADDRESS + (TASK_STACKSIZE*
		GETTCBOFFSET(pstTask->stLink.qwID)));
	// 태스크 설정
	kSetUpTask(pstTask, qwFlags, qwEntryPointAddress, pvStackAddress, TASK_STACKSIZE);

	kInitializeList( &(pstTask->stChildThreadList));

	// FPU 사용 여부를 사용하지 않음으로 초기화
	pstTask->bFPUUsed = FALSE;

	bPreviousFlags = kLockForSystemData();

	// 태스크를 준비 리스트에 추가하여, 스케줄링될 수 있도록 함
	kAddTaskToReadyList(pstTask);


	kUnlockForSystemData(bPreviousFlags);
	return pstTask;
}

static void kSetUpTask(TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress, void* pvStackAddress, QWORD qwStackSize){

	// 콘텍스트 초기화
	kMemSet(pstTCB->stContext.vqwRegister, 0, sizeof(pstTCB->stContext.vqwRegister));

	// RSP, RBP 설정
	pstTCB->stContext.vqwRegister[TASK_RSP_OFFSET] = (QWORD)pvStackAddress + qwStackSize -8;
	pstTCB->stContext.vqwRegister[TASK_RBP_OFFSET] = (QWORD)pvStackAddress + qwStackSize -8;

	// Return Address 영역에 kExitTask() 함수의 어드레스를 삽입하여 태스크의
	// 엔트리 포인트 함수를 빠져나감과 동시에 kExitTask 함수로 이동
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
	pstTCB->qwFlags = qwFlags;
	pstTCB->pvStackAddress = pvStackAddress;
	pstTCB->qwStackSize = qwStackSize;
}

//====================================================================================================
// 스케줄러 관련 함수
//====================================================================================================
// 스케줄러를 초기화
//	스케줄러를 초기화하는데 필요한 TCB 풀과 init 태스크도 같이 초기화
void kInitializeScheduler(void){
	int i;
	TCB* pstTask;
	// TCB 풀 초기화
	kInitializeTCBPool();

	// 준비 리스트 초기화
	for(i=0; i<TASK_MAXREADYLISTCOUNT; i++){
		kInitializeList(&(gs_stScheduler.vstReadyList[i]));
		gs_stScheduler.viExecuteCount[i] = 0;
	}
	kInitializeList( &(gs_stScheduler.stWaitList));

	// TCB를 할당받아 부팅을 수행한 태스크를 커널 최초의 프로세스 설정
	pstTask = kAllocateTCB();
	gs_stScheduler.pstRunningTask = pstTask;
	pstTask->qwFlags = TASK_FLAGS_HIGHEST | TASK_FLAGS_PROCESS | TASK_FLAGS_SYSTEM;
	pstTask->qwParentProcessID = pstTask->stLink.qwID;
	pstTask->pvMemoryAddress = (void*)0x100000;
	pstTask->qwMemorySize = 0x500000;
	pstTask->pvStackAddress = (void*)0x600000;
	pstTask->qwStackSize = 0x100000;

	// 프로세서 사용률을 계산하는데 사용하는 자료구조 초기화
	gs_stScheduler.qwSpendProcessorTimeInIdleTask = 0;
	gs_stScheduler.qwProcessorLoad = 0;
	gs_stScheduler.qwLastFPUUsedTaskID = TASK_INVALIDID;
}

void kSetRunningTask(TCB* pstTask){
	BOOL bPreviousFlags;

	bPreviousFlags = kLockForSystemData();
	gs_stScheduler.pstRunningTask = pstTask;
	kUnlockForSystemData(bPreviousFlags);
}

TCB* kGetRunningTask(void){
	BOOL bPreviousFlags;
	TCB* pstRunningTask;

	bPreviousFlags = kLockForSystemData();
	pstRunningTask = gs_stScheduler.pstRunningTask;
	kUnlockForSystemData(bPreviousFlags);

	return pstRunningTask;
}

static TCB* kGetNextTaskToRun(void){
	BOOL bPreviousFlags;
	TCB* pstTarget = NULL;
	int iTaskCount, i, j;

	// 큐에 태스가 있으나 모든 큐의 태스크가 1회씩 실행된 경우 모든 큐가 프로세서를 앙보하여
	// 태스크를 선택하지 못할 수 있으니 NULL 일 경우 한번 더 수행
	for(j=0; j<2; j++){
		// 높은 우선수위에서 낮은 우선순위까지 리스트를 확인하여 스케줄링할 태스크를 선택
		for(i=0; i<TASK_MAXREADYLISTCOUNT; i++){
			iTaskCount = kGetListCount( &(gs_stScheduler.vstReadyList[i]));

			// 만약 실행한 횟수보다 리스트의 태스크 수가 더 많으면 현재 우선순위의 태스크를 실행함
			if( gs_stScheduler.viExecuteCount[i] < iTaskCount){
				pstTarget = (TCB*) kRemoveListFromHead(
					&(gs_stScheduler.vstReadyList[i]));
				gs_stScheduler.viExecuteCount[i]++;
				break;
			}
			// 만약 실행한 횟수가 더 많으면 실행 횟수를 초기화하고 다음 우선순위로 양보함
			else{
				gs_stScheduler.viExecuteCount[i] = 0;
			}
		}
		if(pstTarget != NULL)
			break;

	}
	return pstTarget;
}

static BOOL kAddTaskToReadyList(TCB* pstTask){
	BYTE bPriority;

	bPriority = GETPRIORITY(pstTask->qwFlags);
	if(bPriority >= TASK_MAXREADYLISTCOUNT)
		return FALSE;

	kAddListToTail( &(gs_stScheduler.vstReadyList[bPriority]), pstTask);
	return TRUE;
}

// 준비 큐에서 태스크를 제거
static TCB* kRemoveTaskFromReadyList(QWORD qwTaskID){
	TCB* pstTarget;
	BYTE bPriority;

	// 태스크 ID가 유효하지 않으면 실패
	if(GETTCBOFFSET(qwTaskID) >= TASK_MAXCOUNT){
		return NULL;
	}

	// TCB 풀에서 해당 태스크의 TCB를 찾아 실제로 ID가 일치하는가 확인
	pstTarget = &(gs_stTCBPoolManager.pstStartAddress[GETTCBOFFSET(qwTaskID)]);
	if(pstTarget->stLink.qwID != qwTaskID)
		return NULL;

	// 태스크가 존재하는 준비 리스트에서 태스크 제거
	bPriority = GETPRIORITY(pstTarget->qwFlags);

	pstTarget = kRemoveList( &(gs_stScheduler.vstReadyList[bPriority]), qwTaskID);
	return pstTarget;
}

// 태스크의 우선순위를 변경함
BOOL kChangePriority(QWORD qwTaskID, BYTE bPriority){
	TCB* pstTarget;
	BOOL bPreviousFlags;

	if(bPriority > TASK_MAXREADYLISTCOUNT)
		return FALSE;

	bPreviousFlags = kLockForSystemData();

	// 현재 실행중인 태스크이면 우선순위만 변경
	// PIT 컨트롤러의 인터럽트(IRQ0) 가 발생하면 태스크 전환이 수행될 때 변경된
	// 우선순위의 리스트로 이동
	pstTarget = gs_stScheduler.pstRunningTask;
	if(pstTarget->stLink.qwID == qwTaskID){
		SETPRIORITY(pstTarget->qwFlags, bPriority);
	}

	// 실행 중인 태스크가 아니면 준비 리스트에서 찾아서 해당 우선수위의 리스트로 이동
	else{
		// 준비 리스트에서 태스크를 찾지 못하면 직접 태스크를 찾아서 우선순위를 설정
		pstTarget = kRemoveTaskFromReadyList(qwTaskID);
		if(pstTarget == NULL){
			// 태스크 ID로 직접 찾아서 설정
			pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
			if(pstTarget != NULL){
				SETPRIORITY(pstTarget->qwFlags, bPriority);
			}
		}
		else{
			// 우선순위를 결정하고 준비 리스트에 다시 삽입
			SETPRIORITY(pstTarget->qwFlags, bPriority);
			kAddTaskToReadyList(pstTarget);
		}
	}
	kUnlockForSystemData(bPreviousFlags);
	return TRUE;
}


void kSchedule(void){
	TCB* pstRunningTask, * pstNextTask;
	BOOL bPreviousFlags;

	if(kGetReadyTaskCount() < 1){
		return;
	}

	bPreviousFlags = kLockForSystemData();

	pstNextTask = kGetNextTaskToRun();
	if(pstNextTask == NULL){
		kUnlockForSystemData(bPreviousFlags);
		return;
	}

	// 현재 수행 중인 태스크의 정보를 수정한 뒤 콘텍스트 전환
	pstRunningTask = gs_stScheduler.pstRunningTask;
	gs_stScheduler.pstRunningTask = pstNextTask;

	if( (pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE){
		gs_stScheduler.qwSpendProcessorTimeInIdleTask += 
		TASK_PROCESSORTIME - gs_stScheduler.iProcessorTime;
	}

	// 다음에 수행할 태스크가 FPU를 쓴 태스크가 아니라면 TS 비트 설정
	if(gs_stScheduler.qwLastFPUUsedTaskID != pstNextTask->stLink.qwID)
		kSetTS();
	else
		kClearTS();


	// 태스크 종료 플래그가 설정된 경우 콘텍스트를 저장할 필요가 없으므로, 대기 리스트에
	// 삽입하고 콘텍스트 전하ㅗㄴ
	if( pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK){
		kAddListToTail( &(gs_stScheduler.stWaitList), pstRunningTask);
		kSwitchContext( NULL, &(pstNextTask->stContext));
	}
	else{
		kAddTaskToReadyList(pstRunningTask);
		kSwitchContext( &(pstRunningTask->stContext), &(pstNextTask->stContext));
	}
	// 프로세서 사용시간 갱신
	gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;

	kUnlockForSystemData(bPreviousFlags);
}

BOOL kScheduleInInterrupt(void){
	TCB* pstRunningTask, * pstNextTask;
	char* pcContextAddress;
	BOOL bPreviousFlags;

	bPreviousFlags = kLockForSystemData();

	pstNextTask = kGetNextTaskToRun();
	if(pstNextTask == NULL){
		kUnlockForSystemData(bPreviousFlags);
		return FALSE;
	}

	// 태스크 전환(인터럽트 핸들러 수행시)
	// - 콘텍스트 저장 : 레지스터->IST의 콘텍스트 메모리(프로세서, ISR가 처리)
	//             IST의 콘텍스트 메모리->런닝 태스크의 콘텍스트 메모리(kMemCpy가 처리)
	// - 콘텍스트 복원 : 넥스트 태스크의 콘텍스트 메모리->IST의 콘텍스트 메모리(kMemCpy가 처리)
	//             IST의 콘텍스트 메모리->레지스터(프로세서, ISR가 처리)
	pcContextAddress = (char*)IST_STARTADDRESS + IST_SIZE - sizeof(CONTEXT);

	// 현재 수행 중인 태스크의 정보를 수정한 뒤 콘텍스트 전환
	pstRunningTask = gs_stScheduler.pstRunningTask;
	gs_stScheduler.pstRunningTask = pstNextTask;

	// 유휴 태스크에서 자ㅓㄴ환되었다면 사용한 프로세서 시간을 증가시킴
	if( (pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE){
		gs_stScheduler.qwSpendProcessorTimeInIdleTask += TASK_PROCESSORTIME;
	}

	// 태스크 종료 플래그가 설정된 경우 콘텍스트를 저장하지 않고 대기 리스트에만 삽입
	if( pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK){
		kAddListToTail( &(gs_stScheduler.stWaitList), pstRunningTask);
	}

	// 태스크가 종료되지 않으면 IST에 있는 콘텍스트를 복사하고, 현재 태스크를 준비 리스트로 옮김
	else{
		kMemCpy( &(pstRunningTask->stContext), pcContextAddress, sizeof(CONTEXT));
		kAddTaskToReadyList(pstRunningTask);
	}
	kUnlockForSystemData(bPreviousFlags);

	// 다음에 수행할 태스크가 FPU를 쓴 태스크가아니면 TS 비트 설정
	if(gs_stScheduler.qwLastFPUUsedTaskID != pstNextTask->stLink.qwID)
		kSetTS();
	else
		kClearTS();

	kMemCpy(pcContextAddress, &(pstNextTask->stContext), sizeof(CONTEXT));

	// 프로세서 사용시간 갱신
	gs_stScheduler.iProcessorTime = TASK_PROCESSORTIME;

	return TRUE;
}

// 태스크를 종료
BOOL kEndTask(QWORD qwTaskID){
	TCB* pstTarget;
	BYTE bPriority;
	BOOL bPreviousFlags;

	// 현재 실행 중인 태스크면 EndTask 비트를 설정하고 태스크를 전환
	bPreviousFlags = kLockForSystemData();

	pstTarget = gs_stScheduler.pstRunningTask;
	if(pstTarget->stLink.qwID == qwTaskID){
		pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
		SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
		kUnlockForSystemData(bPreviousFlags);
		kSchedule();

		// 태스크 전환 되므로 밑에 코드 실행X
		while(1);
	}
	else{
		// 준비 리스트에서 태스크를 찾지 못하면 직접 태스크를 찾아서 태스크 종료 비트를 설정
		pstTarget = kRemoveTaskFromReadyList(qwTaskID);
		if(pstTarget == NULL){
			// 태스크 ID로 직접 찾아서 실행
			pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
			if( pstTarget != NULL){
				pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
				SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
			}
			kUnlockForSystemData(bPreviousFlags);
			return TRUE;
		}
		pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
		SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
		kAddListToTail( &(gs_stScheduler.stWaitList), pstTarget);
	}
	kUnlockForSystemData(bPreviousFlags);
	return TRUE;
}

// 태스크가 자신을 종료함
void kExitTask(void){
	kEndTask(gs_stScheduler.pstRunningTask->stLink.qwID);
}

// 준비 큐에 있는 모든 태스크의 수를 반환
int kGetReadyTaskCount(void){
	int iTotalCount = 0;
	int i;
	BOOL bPreviousFlags;

	bPreviousFlags = kLockForSystemData();

	// 모든 준비 큐를 확인하여 태스크 개수를 구함
	for(i=0; i<TASK_MAXREADYLISTCOUNT; i++){
		iTotalCount += kGetListCount( &(gs_stScheduler.vstReadyList[i]));
	}

	kUnlockForSystemData(bPreviousFlags);
	return iTotalCount;
}

// 전체 태스크의 수를 반환
int kGetTaskCount(void){
	int iTotalCount;
	BOOL bPreviousFlags;

	iTotalCount = kGetReadyTaskCount();

	bPreviousFlags = kLockForSystemData();
	iTotalCount += kGetListCount( &(gs_stScheduler.stWaitList)) + 1;

	kUnlockForSystemData(bPreviousFlags);

	return iTotalCount;
}

// TCB 풀에서 해당 오프셋의 TCB를 반환
TCB* kGetTCBInTCBPool(int iOffset){
	if( (iOffset <= -1) && (iOffset >= TASK_MAXCOUNT)){
		return NULL;
	}

	return &(gs_stTCBPoolManager.pstStartAddress[iOffset]);
}

// 태스크가 존재하는지 여부를 반환
BOOL kIsTaskExist(QWORD qwID){
	TCB* pstTCB;

	// ID로 TCB를 반환
	pstTCB = kGetTCBInTCBPool(GETTCBOFFSET(qwID));
	// TCB가 없거나 ID가 일치하지 않으면 존재하지 않음
	if( (pstTCB == NULL) || (pstTCB->stLink.qwID != qwID))
		return FALSE;

	return TRUE;
}

// 프로세서의 사용률을 반환
QWORD kGetProcessorLoad(void){
	return gs_stScheduler.qwProcessorLoad;
}

static TCB* kGetProcessByThread(TCB *pstThread){
	TCB* pstProcess;

	// 만약 내가 프로세스이면 자신을 반환
	if(pstThread->qwFlags & TASK_FLAGS_PROCESS)
		return pstThread;

	// 내가 프로세스가 아니라면, 부모 프로세스로 설정된 태스크 ID를 통해
	// TCB 풀에서 태스크 자료구조 추출
	pstProcess = kGetTCBInTCBPool(GETTCBOFFSET(pstThread->qwParentProcessID));

	// 만약 프로세스가 없거나, 태스크 ID가 일치하지 않는다면 NULL을 반환

	if( (pstProcess == NULL) || (pstProcess->stLink.qwID != pstThread->qwParentProcessID))
		return NULL;
	return pstProcess;
}	

// 유후 태스크 관련
//	유휴 태스크
//		대기 큐에 삭제 대기중인 태스크를 정리
void kIdleTask(void){
	TCB* pstTask, *pstChildThread, *pstProcess;
	QWORD qwLastMeasureTickCount, qwLastSpendTickInIdleTask;
	QWORD qwCurrentMeasureTickCount, qwCurrentSpendTickInIdleTask;
	BOOL bPreviousFlags;
	int i, iCount;
	QWORD qwTaskID;
	void* pvThreadLink;

	// 프로세서 사용량 계산을 위해 기준 정보를 저장
	qwLastMeasureTickCount = kGetTickCount();
	qwLastSpendTickInIdleTask = gs_stScheduler.qwSpendProcessorTimeInIdleTask;

	while(1){
		// 현재 상태를 저장

		qwCurrentMeasureTickCount = kGetTickCount();
		qwCurrentSpendTickInIdleTask = gs_stScheduler.qwSpendProcessorTimeInIdleTask;

		// 프로세서 사용량을 계산
		// 100 - (유휴 태스크가 사용한 프로세서 시간)*100 /(시스템 전체에서 사용한 프로세서 시간)
		if(qwCurrentMeasureTickCount - qwLastMeasureTickCount ==0)
			gs_stScheduler.qwProcessorLoad = 0;
		else{
			gs_stScheduler.qwProcessorLoad = 100 - ((qwCurrentSpendTickInIdleTask - qwLastSpendTickInIdleTask) * 100 / (qwCurrentMeasureTickCount - qwLastMeasureTickCount));
		} 
		// 현재 상태를 이전 상태에 보관
		qwLastMeasureTickCount = qwCurrentMeasureTickCount;
		qwLastSpendTickInIdleTask = qwCurrentSpendTickInIdleTask;

		// 프로세서의 부하에 따라 쉬게함
		kHaltProcessorByLoad();

		// 대기 큐에 대기중인 태스크가 있으면 태스크를 종료함
		if(kGetListCount( &(gs_stScheduler.stWaitList)) > 0){
			while(1){
				bPreviousFlags = kLockForSystemData();
				pstTask = kRemoveListFromHead( &(gs_stScheduler.stWaitList));
				if(pstTask == NULL){
					kUnlockForSystemData(bPreviousFlags);
					break;
				}

				if(pstTask->qwFlags & TASK_FLAGS_PROCESS){
					// 프로세스를 종료할 때 자식 스레드가 존재하면 스레드를 모두 종료하고,
					// 다시 자식 스레드 리스트에 삽입
					iCount = kGetListCount(&(pstTask->stChildThreadList));
					for(i=0; i<iCount; i++){
						// 스레드 링크의 어드레스에서 꺼내 스레드를 종료함
						pvThreadLink = (TCB*)kRemoveListFromHead(&(pstTask->stChildThreadList));
						if(pvThreadLink == NULL){
							break;
						}

						// 자식 스레드 리스트에 연결된 정보는 태스크 자료구조에 있는
						// stThreadLink의 시작 어드레스이므로, 태스크 자료구조의 시작
						// 어드레스를 구하려면 별도의 계산이 필요함
						pstChildThread = GETTCBFROMTHREADLINK(pvThreadLink);

						// 다시 자식 스레드 리스트에 삽입하여 해당 스레드가 종료될 때
						// 기다려야 하므로 다시 대기 리스트에 삽입
						kAddListToTail( &(pstTask->stChildThreadList), &(pstChildThread->stThreadLink));

						// 자식 스레드를 찾아서 종료
						kEndTask(pstChildThread->stLink.qwID);
					}
					if( kGetListCount( &(pstTask->stChildThreadList)) > 0){
						kAddListToTail( &(gs_stScheduler.stWaitList), pstTask);

						kUnlockForSystemData(bPreviousFlags);
						continue;
					}
						// 프로세스를 종료해야 하므로 할당받은 메모리 영역을 삭제
					else{
						// TODO : 추후에 코드 삽입
					}
				}
				else if(pstTask->qwFlags & TASK_FLAGS_THREAD){
					// 스레드라면 프로세스의 자식 스레드 리스트에서 제거
					pstProcess = kGetProcessByThread(pstTask);
					if(pstProcess != NULL){
						kRemoveList( &(pstProcess->stChildThreadList), pstTask->stLink.qwID);
					}
				}
				qwTaskID = pstTask->stLink.qwID;
				kFreeTCB(qwTaskID);
				kUnlockForSystemData(bPreviousFlags);
				kPrintf("IDLE: Task ID[0x%q] is completely end\n", qwTaskID);
			}
		}
		kSchedule();
	} 
}

// 측정된 프로세서 부하에 따라 프로세서를 쉬게함
void kHaltProcessorByLoad(void){
	if(gs_stScheduler.qwProcessorLoad < 40){
		kHit();
		kHit();
		kHit();
	}
	else if(gs_stScheduler.qwProcessorLoad < 80){
		kHit();
		kHit();
	}
	else if(gs_stScheduler.qwProcessorLoad < 95){
		kHit();
	}
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

// FPU 관련
// 마지막으로 FPU를 사용한 태스크 ID 반환
QWORD kGetLastFPUUsedTaskID(void){
	return gs_stScheduler.qwLastFPUUsedTaskID;
}

// 마지막으로 FPU를 사용한 태스크 ID 설정
void kSetLastFPUUsedTaskID(QWORD qwTaskID){
	gs_stScheduler.qwLastFPUUsedTaskID = qwTaskID;
}