#include "Task.h"
#include "Descriptor.h"
#include "Utility.h"
#include "AssemblyUtility.h"
#include "Synchronization.h"
#include "Console.h"
#include "MultiProcessor.h"

/***** Àü¿ª º¯¼ö Á¤ÀÇ *****/
static SCHEDULER gs_vstScheduler[MAXPROCESSORCOUNT];
static TCBPOOLMANAGER gs_stTCBPoolManager;

//====================================================================================================
// ÅÂ½ºÅ© °ü·Ã ÇÔ¼ö
//====================================================================================================
static void kInitializeTCBPool(void){
	int i;

	// TCB Ç® ¸Å´ÏÀú ÃÊ±âÈ­
	kMemSet(&gs_stTCBPoolManager, 0, sizeof(gs_stTCBPoolManager));

	// TCB Ç® ¿µ¿ª ÃÊ±âÈ­
	gs_stTCBPoolManager.pstStartAddress = (TCB*)TASK_TCBPOOLADDRESS;
	kMemSet((void*)TASK_TCBPOOLADDRESS, 0, sizeof(TCB) * TASK_MAXCOUNT);

	// TCB¿¡ ID(¿ÀÇÁ¼Â) ÇÒ´ç
	for(i = 0; i < TASK_MAXCOUNT; i++){
		gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;
	}

	// TCBÀÇ ÃÖ´ë °³¼ö¿Í ÇÒ´ç È½¼ö ÃÊ±âÈ­
	gs_stTCBPoolManager.iMaxCount = TASK_MAXCOUNT;
	gs_stTCBPoolManager.iAllocatedCount = 1;

	kInitializeSpinLock( &(gs_stTCBPoolManager.stSpinLock));
}

static TCB* kAllocateTCB(void){
	TCB* pstEmptyTCB;
	int i;


	// 동기화 처리
	kLockForSpinLock( &(gs_stTCBPoolManager.stSpinLock));
	if(gs_stTCBPoolManager.iUseCount == gs_stTCBPoolManager.iMaxCount){
		kUnlockForSpinLock( &(gs_stTCBPoolManager.stSpinLock));
		return NULL;
	}

	for(i = 0; i < gs_stTCBPoolManager.iMaxCount; i++){
		// TCB ID(64ºñÆ®)ÀÇ »óÀ§ 32ºñÆ®°¡ 0ÀÌ¸é ÇÒ´çµÇÁö ¾ÊÀº TCB
		if((gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID >> 32) == 0){
			pstEmptyTCB = &(gs_stTCBPoolManager.pstStartAddress[i]);
			break;
		}
	}

	// ÇÒ´çÇÒ TCB IDÀÇ »óÀ§ 32ºñÆ®¸¦ 0ÀÌ ¾Æ´Ñ °ªÀ¸·Î ¼³Á¤ (TCB ID -> »óÀ§ 32ºñÆ®:TCB ÇÒ´ç È½¼ö, ÇÏÀ§ 32ºñÆ®: TCB ¿ÀÇÁ¼Â)
	pstEmptyTCB->stLink.qwID = ((QWORD)gs_stTCBPoolManager.iAllocatedCount << 32) | i;
	gs_stTCBPoolManager.iUseCount++;
	gs_stTCBPoolManager.iAllocatedCount++;

	if(gs_stTCBPoolManager.iAllocatedCount == 0){
		gs_stTCBPoolManager.iAllocatedCount = 1;
	}

	kUnlockForSpinLock( &(gs_stTCBPoolManager.stSpinLock));

	return pstEmptyTCB;
}

static void kFreeTCB(QWORD qwID){
	int i;

	// 태스크 ID의 하위 32비트가 인덱스 영활;
	i = GETTCBOFFSET(qwID);

	kLockForSpinLock( &(gs_stTCBPoolManager.stSpinLock));

	// TCB 초기화하고 ID 설정
	kMemSet(&(gs_stTCBPoolManager.pstStartAddress[i].stContext), 0, sizeof(CONTEXT));
	gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;

	gs_stTCBPoolManager.iUseCount--;

	kUnlockForSpinLock( &(gs_stTCBPoolManager.stSpinLock));
}

TCB* kCreateTask(QWORD qwFlags, void* pvMemoryAddress, QWORD qwMemorySize, QWORD qwEntryPointAddress, BYTE bAffinity){
	TCB* pstTask;    // »ý¼ºÇÒ ÅÂ½ºÅ©(ÇÁ·Î¼¼½º ¶Ç´Â ½º·¹µå)
	TCB* pstProcess; //	void* pvStackAddress;
	void* pvStackAddress;
	BYTE bCurrentAPICID;

	bCurrentAPICID = kGetAPICID();


	// 태스크 자료구조 할당
	pstTask = kAllocateTCB();

	if(pstTask == NULL){
		kUnlockForSpinLock( &(gs_vstScheduler[bCurrentAPICID].stSpinLock));
		return NULL;
	}


	kLockForSpinLock( &(gs_vstScheduler[bCurrentAPICID].stSpinLock));

	// 현재 프로세스나 스레드가 속한 프로세스를 검색
	pstProcess = kGetProcessByThread(kGetRunningTask(bCurrentAPICID));

	// 만약 없다면 아무 작업도 하지 않음
	if(pstProcess == NULL){
		kFreeTCB(pstTask->stLink.qwID);
		kUnlockForSpinLock( &(gs_vstScheduler[bCurrentAPICID].stSpinLock));
		return NULL;
	}

	//****************************************************************************************************
	// ÅÂ½ºÅ© °ü¸® ¸®½ºÆ®ÀÇ µµ½ÄÈ­
	// 1. ÅÂ½ºÅ© »ý¼º½Ã
	//   -------------------
	//   |        -------- |  -> ¹Ù±ùÂÊ »ç°¢ÇüÀº ÁØºñ ¸®½ºÆ®(Ready List)
	//   |P1->P2->|T3=>T4| |  -> ¾ÈÂÊ »ç°¢ÇüÀº ÀÚ½Ä ½º·¹µå ¸®½ºÆ®(Child Thread List)
	//   |        -------- |
	//   -------------------
	// 2. P2 ÇÁ·Î¼¼½º Á¾·á½Ã
	//   ------------
	//   |T3->T4->P2|  -> ´ë±â ¸®½ºÆ®(Wait List)
	//   ------------
	// 3. T3 ½º·¹µå Á¾·á½Ã
	//   ----
	//   |T3|  -> ´ë±â ¸®½ºÆ®(Wait List)
	//   ----
	//****************************************************************************************************

	// ½º·¹µå¸¦ »ý¼ºÇÏ´Â °æ¿ì
	if(qwFlags & TASK_FLAGS_THREAD){

		// [½º·¹µå.ºÎ¸ð ÇÁ·Î¼¼½º ID]¿¡ ÀÚ½ÅÀ» »ý¼ºÇÑ ÇÁ·Î¼¼½º(ºÎ¸ð ÇÁ·Î¼¼½º)ÀÇ ID¸¦ ¼³Á¤ÇÏ°í, [½º·¹µå.¸Þ¸ð¸® ¿µ¿ª]¿¡ ÀÚ½ÅÀ» »ý¼ºÇÑ ÇÁ·Î¼¼½º(ºÎ¸ð ÇÁ·Î¼¼½º)ÀÇ ¸Þ¸ð¸® ¿ª¿ªÀ» ¼³Á¤ÇÔ
		pstTask->qwParentProcessID = pstProcess->stLink.qwID;
		pstTask->pvMemoryAddress = pstProcess->pvMemoryAddress;
		pstTask->qwMemorySize = pstProcess->qwMemorySize;

		// ÀÚ½ÅÀ» »ý¼ºÇÑ ÇÁ·Î¼¼½º(ºÎ¸ð ÇÁ·Î¼¼½º)ÀÇ ÀÚ½Ä ½º·¹µå ¸®½ºÆ®¿¡ »ý¼ºÇÑ ½º·¹µå¸¦ Ãß°¡
		kAddListToTail(&(pstProcess->stChildThreadList), &(pstTask->stThreadLink));

	// ÇÁ·Î¼¼½º¸¦ »ý¼ºÇÏ´Â °æ¿ì
	}else{
		// [ÇÁ·Î¼¼½º.ºÎ¸ð ÇÁ·Î¼¼½º ID]¿¡ ÀÚ½ÅÀ» »ý¼ºÇÑ ÇÁ·Î¼¼½º(ºÎ¸ð ÇÁ·Î¼¼½º)ÀÇ ID¸¦ ¼³Á¤ÇÏ°í, [ÇÁ·Î¼¼½º.¸Þ¸ð¸® ¿µ¿ª]¿¡ ÆÄ¶ó¹ÌÅÍ·Î ³Ñ¾î ¿Â ¸Þ¸ð¸® ¿µ¿ªÀ» ¼³Á¤ÇÔ
		pstTask->qwParentProcessID = pstProcess->stLink.qwID;
		pstTask->pvMemoryAddress = pvMemoryAddress;
		pstTask->qwMemorySize = qwMemorySize;
	}

	// 스레드의 ID를 태스크 ID와 동일하게 설정
	pstTask->stThreadLink.qwID = pstTask->stLink.qwID;

	kUnlockForSpinLock( &(gs_vstScheduler[bCurrentAPICID].stSpinLock));

	// 태스크 ID로 스택어드레스 계산. 하위 32비트가 스택 풀의 오프셋 역할 수행
	pvStackAddress = (void*)(TASK_STACKPOOLADDRESS + (TASK_STACKSIZE * GETTCBOFFSET(pstTask->stLink.qwID)));

	// TCB를 설정한 후 준비 리스트에 삽입하여 스케줄링 될 수 있도록 함
	kSetUpTask(pstTask, qwFlags, qwEntryPointAddress, pvStackAddress, TASK_STACKSIZE);

	// 자식 스레드 리스트를 초기화
	kInitializeList(&(pstTask->stChildThreadList));

	// FPU 사용 여부를 사용하지 않는 것으로 초기화
	pstTask->bFPUUsed = FALSE;

	// 현재 코어의 로컬 APIC ID를 태스크에 설정
	pstTask->bAPICID = bCurrentAPICID;

	// 프로세서 친화도를 설정
	pstTask->bAffinity = bAffinity;

	// 부하 분산을 고려하여 스케줄러에 태스크를 추가
	kAddTaskToSchedulerWithLoadBalancing(pstTask);

	return pstTask;
}

static void kSetUpTask(TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress, void* pvStackAddress, QWORD qwStackSize){

	// ÄÜÅØ½ºÆ® ÃÊ±âÈ­
	kMemSet(pstTCB->stContext.vqwRegister, 0, sizeof(pstTCB->stContext.vqwRegister));

	// RSP, RBP ¼³Á¤
	pstTCB->stContext.vqwRegister[TASK_RSP_OFFSET] = (QWORD)pvStackAddress + qwStackSize - 8;
	pstTCB->stContext.vqwRegister[TASK_RBP_OFFSET] = (QWORD)pvStackAddress + qwStackSize - 8;

	// ½ºÅÃÀÇ ÃÖ»óÀ§ 8¹ÙÀÌÆ® ¿µ¿ª¿¡ º¹±Í ÁÖ¼Ò·Î¼­  kExitTask()ÇÔ¼öÀÇ ¾îµå·¹½º¸¦ »ðÀÔÇÔÀ¸·Î½á,
	// ÅÂ½ºÅ©ÀÇ ¿£Æ®¸® Æ÷ÀÎÆ® ÇÔ¼ö°¡ return µÇ´Â ¼ø°£, kExitTask()ÇÔ¼ö·Î ÀÌµ¿ÇÏ°Ô ÇÔ
	*(QWORD*)((QWORD)pvStackAddress + qwStackSize - 8) = (QWORD)kExitTask;

	// ¼¼±×¸ÕÆ® ¼¿·ºÅÍ ¼³Á¤
	pstTCB->stContext.vqwRegister[TASK_CS_OFFSET] = GDT_KERNELCODESEGMENT;
	pstTCB->stContext.vqwRegister[TASK_DS_OFFSET] = GDT_KERNELDATASEGMENT;
	pstTCB->stContext.vqwRegister[TASK_ES_OFFSET] = GDT_KERNELDATASEGMENT;
	pstTCB->stContext.vqwRegister[TASK_FS_OFFSET] = GDT_KERNELDATASEGMENT;
	pstTCB->stContext.vqwRegister[TASK_GS_OFFSET] = GDT_KERNELDATASEGMENT;
	pstTCB->stContext.vqwRegister[TASK_SS_OFFSET] = GDT_KERNELDATASEGMENT;

	// RIP, RFLAGS ¼³Á¤
	pstTCB->stContext.vqwRegister[TASK_RIP_OFFSET] = qwEntryPointAddress;
	pstTCB->stContext.vqwRegister[TASK_RFLAGS_OFFSET] |= 0x0200; // RFLAGSÀÇ IF(ºñÆ® 9)=1·Î ¼³Á¤ÇÏ¿©, ÀÎÅÍ·´Æ® È°¼ºÈ­

	// ±âÅ¸ ¼³Á¤
	pstTCB->pvStackAddress = pvStackAddress;
	pstTCB->qwStackSize = qwStackSize;
	pstTCB->qwFlags = qwFlags;
}

//====================================================================================================
// ½ºÄÉÁÙ·¯ °ü·Ã ÇÔ¼ö
//====================================================================================================
void kInitializeScheduler(void){
	int i, j;
	BYTE bCurrentAPICID;
	TCB* pstTask;

	// 현재 APIC ID 확인
	bCurrentAPICID = kGetAPICID();

	if(bCurrentAPICID == 0){
		// 태스크 풀 초기화
		kInitializeTCBPool();

		// 준비 리스트와 우선순위별 실행 횟수를 초기화 하고 대기 리스트와 스핀락을 초기화
		for(j=0; j< MAXPROCESSORCOUNT; j++){
			// 준비 리스트 초기화
			for(i = 0; i < TASK_MAXREADYLISTCOUNT; i++){
				kInitializeList(&(gs_vstScheduler[j].vstReadyList[i]));
				gs_vstScheduler[j].viExecuteCount[i] = 0;
			}
		}
		// 대기 리스트 초기화
		kInitializeList(&(gs_vstScheduler[j].stWaitList));

		// 스핀락 초기화
		kInitializeSpinLock( &(gs_vstScheduler[j].stSpinLock));
	}


	

	// TCB 할당받아 부팅을 수행한 태스크를 커널 최초의 프로세스로 설정
	pstTask = kAllocateTCB();
	gs_vstScheduler[bCurrentAPICID].pstRunningTask = pstTask;

	// BSP의 콘솔 셸이나 AP의 유휴 태스크는 모두 현재 코어에서만 실행하도록
	// 로컬 APIC ID와 프로세서 친화도를 현재 코어의 로컬 APIC ID로 설정
	pstTask->bAPICID = bCurrentAPICID;
	pstTask->bAffinity = bCurrentAPICID;

	// Bootstrap Processor는 콘솔 셸을 실행
	if(bCurrentAPICID == 0)
		pstTask->qwFlags = TASK_FLAGS_HIGHEST | TASK_FLAGS_PROCESS | TASK_FLAGS_SYSTEM; // ºÎÆÃÀ» ¼öÇàÇÑ ÅÂ½ºÅ©(ÀÌÈÄ¿¡  ÄÜ¼Ö ½© ÅÂ½ºÅ©°¡ µÊ)ÀÇ ¿ì¼±¼øÀ§¸¦ °¡Àå ³ôÀ½À¸·Î ¼³Á¤

	// Application Processor 특별히 긴급한 태스크가 없으므로 유휴 태스크를 실행
	else
		pstTask->qwFlags = TASK_FLAGS_LOWEST | TASK_FLAGS_PROCESS | TASK_FLAGS_SYSTEM | TASK_FLAGS_IDLE;

	pstTask->qwParentProcessID = pstTask->stLink.qwID; // Ä¿³Î ÃÖÃÊ ÇÁ·Î¼¼½º´Â ºÎ¸ð ÇÁ·Î¼¼½º°¡ Á¸Àç ÇÏÁö ¾ÊÀ½À¸·Î, ºÎ¸ð ÇÁ·Î¼¼½º ID¿¡ ÀÚ½ÅÀÇ ID¸¦ ¼³Á¤
	pstTask->pvMemoryAddress = (void*)0x100000;
	pstTask->qwMemorySize = 0x500000;
	pstTask->pvStackAddress = (void*)0x600000;
	pstTask->qwStackSize = 0x100000;

	// 프로세서 사용률을 계산하는데 사용하는 자료구조 초기화
	gs_vstScheduler[bCurrentAPICID].qwSpendProcessorTimeInIdleTask = 0;
	gs_vstScheduler[bCurrentAPICID].qwProcessorLoad = 0;

	// FPU 사용한 태스크 ID를 유효하지 않은 값으로 초기화
	gs_vstScheduler[bCurrentAPICID].qwLastFPUUsedTaskID = TASK_INVALIDID;
}

// 현재 수행 중인 태스크를 설정
void kSetRunningTask(BYTE bAPICID, TCB* pstTask){
	BOOL bPreviousFlag;

	kLockForSpinLock( &(gs_vstScheduler[bAPICID].stSpinLock));

	gs_vstScheduler[bAPICID].pstRunningTask = pstTask;

	kUnlockForSpinLock( &(gs_vstScheduler[bAPICID].stSpinLock));
}

// 현재 수행중인 태스크 반환
TCB* kGetRunningTask(BYTE bAPICID){
	TCB* pstRunningTask;
	BOOL bPreviousFlag;

	kLockForSpinLock( &(gs_vstScheduler[bAPICID].stSpinLock));


	pstRunningTask = gs_vstScheduler[bAPICID].pstRunningTask;

	kUnlockForSpinLock( &(gs_vstScheduler[bAPICID].stSpinLock));

	return pstRunningTask;
}

// 태스크 리스트에서 다음으로 실행될 태스크를 얻음
static TCB* kGetNextTaskToRun(BYTE bAPICID){
	TCB* pstTarget = NULL;
	int iTaskCount;
	int i, j;

	// 큐에 태스크가 있으나 모든 큐의 태스크가 1회씩 실행된 경우, 모든 큐가 프로세서를 양보하여
	// 태스크를 선택하지 못할 수 있으니 NULL 이면 한번더 수행
	for(j=0; j<2; j++){

		for(i = 0; i < TASK_MAXREADYLISTCOUNT; i++){
			iTaskCount = kGetListCount(&(gs_vstScheduler[bAPICID].vstReadyList[i]));

			// 만약 실행한 횟수보다 리스트의 태스크가 더 많으면 현재 우선순위의
			// 태스크를 실행함
			if(gs_vstScheduler[bAPICID].viExecuteCount[i] < iTaskCount){
				pstTarget = (TCB*)kRemoveListFromHeader(&(gs_vstScheduler[bAPICID].vstReadyList[i]));
				gs_vstScheduler[bAPICID].viExecuteCount[i]++;
				break;

			// 만약 실행한 횟수가 더 많으면 실행 횟수를 초기화하고 다음 우선순위로 양보함
			}
			else{
				gs_vstScheduler[bAPICID].viExecuteCount[i] = 0;
			}

		}

		// 만약 수행할 태스크를 찾았으면 종료
		if(pstTarget != NULL){
			break;
		}
	}

	return pstTarget;
}

static BOOL kAddTaskToReadyList(BYTE bAPICID, TCB* pstTask){
	BYTE bPriority;

	bPriority = GETPRIORITY(pstTask->qwFlags);

	if(bPriority >= TASK_MAXREADYLISTCOUNT){
		return FALSE;
	}

	kAddListToTail(&(gs_vstScheduler[bAPICID].vstReadyList[bPriority]), pstTask);
	return TRUE;
}

static TCB* kRemoveTaskFromReadyList(BYTE bAPICID, QWORD qwTaskID){
	TCB* pstTarget;
	BYTE bPriority;

	// 태스크 ID가 유효하지 않으면 실패
	if(GETTCBOFFSET(qwTaskID) >= TASK_MAXCOUNT){
		return NULL;
	}

	// TCB 풀에서 해당 태스크의 TCB를 찾아 실제로 ID가 일치하는가 확인
	pstTarget = &(gs_stTCBPoolManager.pstStartAddress[GETTCBOFFSET(qwTaskID)]);
	if(pstTarget->stLink.qwID != qwTaskID){
		return NULL;
	}

	bPriority = GETPRIORITY(pstTarget->qwFlags);

	if( bPriority >= TASK_MAXREADYLISTCOUNT ){
	        return NULL;
	}   

	pstTarget = kRemoveList(&(gs_vstScheduler[bAPICID].vstReadyList[bPriority]), qwTaskID);

	return pstTarget;
}

// 태스크가 포함된 스케줄러의 ID를 반환하고, 해당 스케줄러의 스핀락을 잠금
static BOOL kFindSchedulerOfTaskAndLock(QWORD qwTaskID, BYTE* pbAPICID){
	TCB* pstTarget;
	BYTE bAPICID;

	while(1){
		// 태스크 ID로 태스크 자료구조를 찾아서 어느 스케줄러에서 실행 중인지 확인
		pstTarget = &(gs_stTCBPoolManager.pstStartAddress[GETTCBOFFSET(qwTaskID)]);
		if( (pstTarget == NULL) || (pstTarget->stLink.qwID != qwTaskID))
			return FALSE;

		// 현재 태스크가 실행되는 코어의 ID 확인
		bAPICID = pstTarget->bAPICID;

		// 임계 영역 시작
		kLockForSpinLock( &(gs_vstScheduler[bAPICID].stSpinLock));

		// 스핀락을 흭득한 이후 다시 확인하여 같은 코어에서 실행되는지 확인
		// 태스크 수행되는 코어를 찾은후 정확하게 스핀락을 걸기 위해 이중으로 검사
		pstTarget = &(gs_stTCBPoolManager.pstStartAddress[GETTCBOFFSET(qwTaskID)]);
		if(pstTarget->bAPICID == bAPICID)
			break;

		// 태스크 자료구조에 저장된 로컬 APIC ID 값이 스핀락을 흭득하기 전과 후가 다르면,
		// 스핀락을 흭득하는 동안 태스크가 다른 코어로 옮겨간 것임
		// 따라서 다시 스핀락을 해제하고 옮겨진 코어의 스핀락을 흭득해야 함
		// 임계 영역 끝
		kUnlockForSpinLock( &(gs_vstScheduler[bAPICID].stSpinLock));
	}

	*pbAPICID = bAPICID;
	return TRUE;
}

BOOL kChangePriority(QWORD qwTaskID, BYTE bPriority){
	TCB* pstTarget;
	BOOL bPreviousFlag;
	BYTE bAPICID;

	if(bPriority >= TASK_MAXREADYLISTCOUNT){
		return FALSE;
	}

	if(kFindSchedulerOfTaskAndLock(qwTaskID, &bAPICID) == FALSE)
		return FALSE;

	// 실행 중인 태스크이면 우선순위만 변경
	// PIT 컨트롤러의 인터럽트(IRQ 0)가 발생하여 태스크 전환이 수행될 때 변경된
	// 우선순위의 리스트로 이동
	pstTarget = gs_vstScheduler[bAPICID].pstRunningTask;

	if(pstTarget->stLink.qwID == qwTaskID){
		SETPRIORITY(pstTarget->qwFlags, bPriority);

    // 실행중인 태스크가 아니면 준비 리스트에서 찾아서 해당 우선순위의 리스트로 이동
	}else{
		pstTarget = kRemoveTaskFromReadyList(bAPICID, qwTaskID);

		// 준비 리스트에서 태스크를 찾지 못하면 직접 태스크를 찾아서 우선순위를 설정
		if(pstTarget == NULL){
			pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
			if(pstTarget != NULL){
				SETPRIORITY(pstTarget->qwFlags, bPriority);
			}
		}else{
			// 우선순위를 설정하고 준비 리스트에 다시 삽입
			SETPRIORITY(pstTarget->qwFlags, bPriority);
			kAddTaskToReadyList(bAPICID ,pstTarget);
		}
	}

	kUnlockForSpinLock( &(gs_vstScheduler[bAPICID].stSpinLock));

	return TRUE;
}

BOOL kSchedule(void){
	TCB* pstRunningTask, * pstNextTask;
	BOOL bPreviousFlag;
	BYTE bCurrentAPICID;

	// 전환하는 도중 인터럽트가 발생하여 태스크 전환이 또 일어나면 곤란하므로 전환하는 동안
	// 인터럽트가 발생하지 못하도록 설정
	bPreviousFlag = kSetInterruptFlag(FALSE);

	bCurrentAPICID = kGetAPICID();

	if(kGetReadyTaskCount(bCurrentAPICID) < 1){
		kSetInterruptFlag(bPreviousFlag);
		return FALSE;
	}

	kLockForSpinLock( &(gs_vstScheduler[bCurrentAPICID].stSpinLock));

	pstNextTask = kGetNextTaskToRun(bCurrentAPICID);
	if(pstNextTask == NULL){
		kUnlockForSpinLock(&(gs_vstScheduler[bCurrentAPICID].stSpinLock));
		kSetInterruptFlag(bPreviousFlag);
		return;
	}

	// 현재 수행 중인 태스크의 정보를 수정한 뒤 콘텍스트 전환
	pstRunningTask = gs_vstScheduler[bCurrentAPICID].pstRunningTask;
	gs_vstScheduler[bCurrentAPICID].pstRunningTask = pstNextTask;

	// 유휴 태스크에서 전환되었다면 사용한 프로세서 시간을 증가 시킴
	if((pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE){
		gs_vstScheduler[bCurrentAPICID].qwSpendProcessorTimeInIdleTask += (TASK_PROCESSORTIME - gs_vstScheduler[bCurrentAPICID].iProcessorTime);
	}

	// 다음에 수행할 태스크가 FPU를 쓴 태스크가 아니라면 TS 비트를 설정
	if(gs_vstScheduler[bCurrentAPICID].qwLastFPUUsedTaskID != pstNextTask->stLink.qwID){
		kSetTS();

	}else{
		kClearTS();
	}

	// 태스크 종료 플래그가 설정되면 콘텍스트를 저장할 필요가 없으므로, 대기 리스트에
	// 삽입하고 콘텍스트 전환
	if((pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK)){
		kAddListToTail(&(gs_vstScheduler[bCurrentAPICID].stWaitList), pstRunningTask);
		kUnlockForSpinLock( &(gs_vstScheduler[bCurrentAPICID].stSpinLock));
		kSwitchContext(NULL, &(pstNextTask->stContext));

	}else{
		kAddTaskToReadyList(bCurrentAPICID ,pstRunningTask);
		kUnlockForSpinLock( &(gs_vstScheduler[bCurrentAPICID].stSpinLock));
		kSwitchContext(&(pstRunningTask->stContext), &(pstNextTask->stContext));
	}

	// 프로세서 사용시간을 업데이트
	gs_vstScheduler[bCurrentAPICID].iProcessorTime = TASK_PROCESSORTIME;

	kSetInterruptFlag(bPreviousFlag);

	return FALSE;
}

// 인터럽가 발생했을 때 다른 태스크를 찾아 전환
// 반드시 인터럽트나 예외가 발생했을 때 호출 해야 함
BOOL kScheduleInInterrupt(void){
	TCB* pstRunningTask, *pstNextTask;
	char* pcContextAddress;
	BYTE bCurrentAPICID;
	QWORD qwISTStartAddress;

	bCurrentAPICID = kGetAPICID();

	kLockForSpinLock( &(gs_vstScheduler[bCurrentAPICID].stSpinLock));

	pstNextTask = kGetNextTaskToRun(bCurrentAPICID);
	if(pstNextTask == NULL){
		kUnlockForSpinLock( &(gs_vstScheduler[bCurrentAPICID].stSpinLock));
		return FALSE;
	}

	//****************************************************************************************************
	// 태스크 전환 처리
	//    인터럽트 핸들러에서 저장한 콘텍스트를 다른 콘텍스트로 덮어쓰는 방법으로 처리
	//****************************************************************************************************

	qwISTStartAddress = IST_STARTADDRESS + IST_SIZE - (IST_SIZE / MAXPROCESSORCOUNT * bCurrentAPICID);
	pcContextAddress = (char*)qwISTStartAddress - sizeof(CONTEXT);
	pstRunningTask = gs_vstScheduler[bCurrentAPICID].pstRunningTask;
	gs_vstScheduler[bCurrentAPICID].pstRunningTask = pstNextTask;

	// 유휴 태스크에서 전환되었다면 사용한 틱 카운터를 증가시킴
	if((pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE){
		gs_vstScheduler[bCurrentAPICID].qwSpendProcessorTimeInIdleTask += TASK_PROCESSORTIME;
	}

	// 태스크 종료 플래그가 설정되면 콘텍스트를 저장하지 않고 대기 리스트에만 삽입
	if((pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK) == TASK_FLAGS_ENDTASK){
		kAddListToTail(&(gs_vstScheduler[bCurrentAPICID].stWaitList), pstRunningTask);

	// 태스크가 종료되지 않으면 IST에 있는 콘텍스트를 복사하고, 현재 태스크를 준비 리스트로 옮김
	}else{
		kMemCpy(&(pstRunningTask->stContext), pcContextAddress, sizeof(CONTEXT));
	}


	// 다음에 수행할 태스크가 FPU 쓴 태스크가 아니라면 TS 설정
	if(gs_vstScheduler[bCurrentAPICID].qwLastFPUUsedTaskID != pstNextTask->stLink.qwID){
		kSetTS();

	}else{
		kClearTS();
	}

	kUnlockForSpinLock( &(gs_vstScheduler[bCurrentAPICID].stSpinLock));

	kMemCpy(pcContextAddress, &(pstNextTask->stContext), sizeof(CONTEXT));


	// 죵료하는 태스크가 아니면 스케줄러에 태스크 추가
	if( (pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK) != TASK_FLAGS_ENDTASK)
		kAddTaskToSchedulerWithLoadBalancing(pstRunningTask);

	// 프로세서 사용 시간을 업데이트
	gs_vstScheduler[bCurrentAPICID].iProcessorTime = TASK_PROCESSORTIME;

	return TRUE;
}

void kDecreaseProcessorTime(BYTE bAPICID){
	gs_vstScheduler[bAPICID].iProcessorTime--;
}

BOOL kIsProcessorTimeExpired(BYTE bAPICID){
	if(gs_vstScheduler[bAPICID].iProcessorTime <= 0){
		return TRUE;
	}

	return FALSE;
}

BOOL kEndTask(QWORD qwTaskID){
	TCB* pstTarget;
	BYTE bPriority;
	BYTE bAPICID;

	if( kFindSchedulerOfTaskAndLock(qwTaskID, &bAPICID) == FALSE)
		return FALSE;


	// 현재 실행 중인 태스크면 EndTask 비트를 설정하고 태스크를 전환
	pstTarget = gs_vstScheduler[bAPICID].pstRunningTask;
	if(pstTarget->stLink.qwID == qwTaskID){
		pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
		SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);

		kUnlockForSpinLock( &(gs_vstScheduler[bAPICID].stSpinLock));

		// 같은 APIC 만 태스크 전환
		if(kGetAPICID() == bAPICID){
			kSchedule();
			while(1);
		}

		return TRUE;

	// 실행 중인 태스크가 아니면 준비 큐에서 직접 찾아서 대기 리스트에 연결
	// 준비 리스트에서 태스크를 찾지 못하면 직접 태스크를 찾아서 태스크 종료 비트를 설정
	}
	pstTarget = kRemoveTaskFromReadyList(bAPICID ,qwTaskID);

	if(pstTarget == NULL){
		pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
		if(pstTarget != NULL){
			pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
			SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
		}

		kUnlockForSpinLock( &(gs_vstScheduler[bAPICID].stSpinLock));
		return TRUE;
	}

	pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
	SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);
	kAddListToTail(&(gs_vstScheduler[bAPICID].stWaitList), pstTarget);

	kUnlockForSpinLock( &(gs_vstScheduler[bAPICID].stSpinLock));
	return TRUE;
}

void kExitTask(void){
	kEndTask(gs_vstScheduler[kGetAPICID()].pstRunningTask->stLink.qwID);
}

int kGetReadyTaskCount(BYTE bAPICID){
	int iTotalCount = 0;
	int i;

	kLockForSpinLock( &(gs_vstScheduler[bAPICID].stSpinLock));

	for(i = 0; i < TASK_MAXREADYLISTCOUNT; i++){
		iTotalCount += kGetListCount(&(gs_vstScheduler[bAPICID].vstReadyList[i]));
	}

	kUnlockForSpinLock( &(gs_vstScheduler[bAPICID].stSpinLock));

	return iTotalCount;
}

int kGetTaskCount(BYTE bAPICID){
	int iTotalCount;

	//ÃÑ ÅÂ½ºÅ© ¼ö = ½ÇÇà ÁØºñ ÅÂ½ºÅ© ¼ö + Á¾·á ´ë±â ÅÂ½ºÅ© ¼ö + ÇöÀç ½ÇÇàÁßÀÎ ÅÂ½ºÅ© ¼ö
	iTotalCount = kGetReadyTaskCount(bAPICID);

	kLockForSpinLock( &(gs_vstScheduler[bAPICID].stSpinLock));

	iTotalCount += kGetListCount(&(gs_vstScheduler[bAPICID].stWaitList)) + 1;

	kUnlockForSpinLock( &(gs_vstScheduler[bAPICID].stSpinLock));

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

QWORD kGetProcessorLoad(BYTE bAPICID){
	return gs_vstScheduler[bAPICID].qwProcessorLoad;
}

static TCB* kGetProcessByThread(TCB* pstThread){
	TCB* pstProcess;

	// ÆÄ¶ó¹ÌÅÍ°¡ ÇÁ·Î¼¼½ºÀÎ °æ¿ì, ÀÚ½ÅÀ» ¸®ÅÏ
	if(pstThread->qwFlags & TASK_FLAGS_PROCESS){
		return pstThread;
	}

	// ÆÄ¶ó¹ÌÅÍ°¡ ½º·¹µåÀÎ °æ¿ì, ºÎ¸ð ÇÁ·Î¼¼½º¸¦ ¸®ÅÏ
	pstProcess = kGetTCBInTCBPool(GETTCBOFFSET(pstThread->qwParentProcessID));
	if((pstProcess == NULL) || (pstProcess->stLink.qwID != pstThread->qwParentProcessID)){
		return NULL;
	}

	return pstProcess;
}

// 각 스케줄러의 태스크 수를 이용하여 적절한 스케줄러에 추가
// 부하 분산 기능을 사용하지 않는 경우 현재 코어에 삽입
// 부하 분산을 사용하는 경우, 태스크가 현재 수행되는 코어에서 계속 수행하므로
// pstTask에는 적어도 APIC ID가 설정되어 있어야 함
void kAddTaskToSchedulerWithLoadBalancing(TCB* pstTask){
	BYTE bCurrentAPICID;
	BYTE bTargetAPICID;

	// 태스크가 동작하던 코어의 APIC를 확인
	bCurrentAPICID = pstTask->bAPICID;

	// 부하 분산기능을 사용하고, 프로세서 친화도가 모든 코어(0xff)
	// 설정되었으면 부하 분산 수행
	if( (gs_vstScheduler[bCurrentAPICID].bUseLoadBalancing == TRUE) && (pstTask->bAffinity == TASK_LOADBALANCINGID))
		// 태스크를 추가할 스케줄러의 선택
		bTargetAPICID = kFindSchedulerOfMinimumTaskCount(pstTask);

	//태스크 부하 분산기능과 관계 없이 프로세서 친화도 필드에 다른 코어의 APIC ID가
	//들어 있으면 해당 스케줄러로 옮겨줌
	else if( (pstTask->bAffinity != bCurrentAPICID) && (pstTask->bAffinity != TASK_LOADBALANCINGID))
		bTargetAPICID = pstTask->bAffinity;
	// 부하 분산을 사용하지 않는 경우는 현재 스케줄러에 다시 삽입
	else
		bTargetAPICID = bCurrentAPICID;

	kLockForSpinLock( &(gs_vstScheduler[bCurrentAPICID].stSpinLock));

	// 태스크를 추가할 스케줄러가 현재 스케줄러와 다르면 태스크를 이동
	// FPU 공유되지 않으므로 현재 태스크 FPU 마지막으로 썻다면 FPU 콘텍스트를
	// 메모리에 저장해야함
	if( (bCurrentAPICID != bTargetAPICID) && (pstTask->stLink.qwID == gs_vstScheduler[bCurrentAPICID].qwLastFPUUsedTaskID)){
		// FPU 저장전에 TS 비트를 끄지 않으면, 예외 7 (device not available) 발생
		kClearTS();
		kSaveFPUContext(pstTask->vqwFPUContext);
		gs_vstScheduler[bCurrentAPICID].qwLastFPUUsedTaskID = TASK_INVALIDID;
	}

	kUnlockForSpinLock( &(gs_vstScheduler[bCurrentAPICID].stSpinLock));

	kLockForSpinLock( &(gs_vstScheduler[bTargetAPICID].stSpinLock));

	pstTask->bAPICID = bTargetAPICID;
	kAddTaskToReadyList(bTargetAPICID, pstTask);

	kUnlockForSpinLock( &(gs_vstScheduler[bTargetAPICID].stSpinLock));
	/*
	왜 굳이 같은 스케줄러의 락을 사용해야할까
	또한 타켓의 스케줄러의 락을 이용해야 할까
	*/
}


// 태스크를 추가할 스케줄러의 ID를 반환
// 파라미터로 전달된 태스크 자료구조에는 적어도 플래그와 프로세서 친화도 필드가 채워져 있어야함
static BYTE kFindSchedulerOfMinimumTaskCount(const TCB* pstTask){
	BYTE bPriority;
	BYTE i;
	int iCurrentTaskCount;
	int iMinTaskCount;
	BYTE bMinCoreIndex;
	int iTempTaskCount;
	int iProcessorCount;

	iProcessorCount = kGetProcessorCount();

	if(iProcessorCount == 1)
		return pstTask->bAPICID;

	// 우선 순위 추출
	bPriority = GETPRIORITY(pstTask->qwFlags);

	// 태스크가 포함된 스케줄러에서 태스크와 같은 우선순우의 태스크 수를 확인
	iCurrentTaskCount = kGetListCount( &(gs_vstScheduler[pstTask->bAPICID].vstReadyList[bPriority]));

	// 나머지 코어에서 같은 현재 태스크와 같은 레벨을 검사
	// 자신과 태스크의 수가 적어도 2 이상 차이 나는 것 중에서 태스크 수가 작은
	// 스케줄러의 ID 반환
	iMinTaskCount = TASK_MAXCOUNT;
	bMinCoreIndex = pstTask->bAPICID;

	for(i=0; i<iProcessorCount; i++){
		if(i == pstTask->bAPICID)
			continue;

		// 모든 스케줄러를 돌면서 확인
		iTempTaskCount = kGetListCount( &(gs_vstScheduler[i].vstReadyList[bPriority]));

		// 현재 코어와 태스크 수가 2개 이상 차이가 나고 이전까지 태스크 수가
		// 이전 보다 작다면 정보 갱신
		if( (iTempTaskCount+2 <= iCurrentTaskCount) && (iTempTaskCount < iMinTaskCount)){
			bMinCoreIndex = i;
			iMinTaskCount = iTempTaskCount;
		}
	}

	return bMinCoreIndex;
}

// 파라미터로 전달된 코어에 태스크 부하 분산 기능 사용 여부를 설정
BYTE kSetTaskLoadBalancing(BYTE bAPICID, BOOL bUseLoadBalancing){
	gs_vstScheduler[bAPICID].bUseLoadBalancing = bUseLoadBalancing;
}

// 프로세서 친화도를 변경
BOOL kChangeProcessorAffinity(QWORD qwTaskID, BYTE bAffinity){
	TCB* pstTarget;
	BYTE bAPICID;

	// 태스크가 포함된 코어의 로컬 APIC ID를 찾은 후 스핀락을 잠금
	if(kFindSchedulerOfTaskAndLock(qwTaskID, &bAPICID) == FALSE){
		return FALSE;
	}

	// 현재 실행 중인 태스크이면 프로세서 친화도만 변경. 실제 태스크가 옮겨지는 시점은
	// 태스크 전환이 수행될 때
	pstTarget = gs_vstScheduler[bAPICID].pstRunningTask;
	if(pstTarget->stLink.qwID == qwTaskID){
		pstTarget->bAffinity = bAffinity;
		kUnlockForSpinLock( &(gs_vstScheduler[bAPICID].stSpinLock));
	}

	// 실행중인 프로세서가 아니면
	else{
		// 준비 리스트에서 태스크를 찾지 못하면 직접 태스크를 찾아서 진화도를 설정
		pstTarget = kRemoveTaskFromReadyList(bAPICID, qwTaskID);
		if(pstTarget == NULL){
			pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
			if(pstTarget != NULL){
				pstTarget->bAffinity = bAffinity;
			}
		}
		else{
			pstTarget->bAffinity = bAffinity;
		}

		kUnlockForSpinLock( &(gs_vstScheduler[bAPICID].stSpinLock));

		// 프로세서 부하 분산을 고려해서 스케줄러에 등록
		kAddTaskToSchedulerWithLoadBalancing(pstTarget);
	}

	return TRUE;
}

//====================================================================================================
// 유휴 태스크 관련
//====================================================================================================
// 대기큐에 삭제 대기중인 태스크를 정리
void kIdleTask(void){
	TCB* pstTask, * pstChildThread, * pstProcess;
	QWORD qwLastMeasureTickCount, qwLastSpendTickInIdleTask;
	QWORD qwCurrentMeasureTickCount, qwCurrentSpendTickInIdleTask;
	QWORD qwTaskID, qwChildThreadID;
	BOOL bPreviousFlag;
	int i, iCount;
	void* pvThreadLink;
	BYTE bCurrentAPICID;
	BYTE bProcessAPICID;


	bCurrentAPICID = kGetAPICID();

	// 프로세서 사용량 계산을 위해 기준 정보를 저장
	qwLastSpendTickInIdleTask = gs_vstScheduler[bCurrentAPICID].qwSpendProcessorTimeInIdleTask;
	qwLastMeasureTickCount = kGetTickCount();

	while(1){

		//----------------------------------------------------------------------------------------------------
		// 1. 현재 상태를 저장
		//----------------------------------------------------------------------------------------------------
		qwCurrentMeasureTickCount = kGetTickCount();
		qwCurrentSpendTickInIdleTask = gs_vstScheduler[bCurrentAPICID].qwSpendProcessorTimeInIdleTask;

		// 프로세서 사용량을 계산
		// 100 - (유휴 태스크가 사용한 프로세서 시간) * 100 / (시스템 전체에서 사용한 프로세서 시간)
		if((qwCurrentMeasureTickCount - qwLastMeasureTickCount) == 0){
			gs_vstScheduler[bCurrentAPICID].qwProcessorLoad = 0;

		}else{
			gs_vstScheduler[bCurrentAPICID].qwProcessorLoad = 100 - ((qwCurrentSpendTickInIdleTask - qwLastSpendTickInIdleTask) * 100 / (qwCurrentMeasureTickCount - qwLastMeasureTickCount));
		}

		// 현재 상태를 이전 상태에 보관

		qwLastMeasureTickCount = qwCurrentMeasureTickCount;
		qwLastSpendTickInIdleTask = qwCurrentSpendTickInIdleTask;

		// 프로세서의 부하에 따라 쉬게함
		kHaltProcessorByLoad(bCurrentAPICID);


		// 대기 큐에 대기중인 태스크가 있으면 태스크를 종료함
		if(kGetListCount(&(gs_vstScheduler[bCurrentAPICID].stWaitList)) > 0){
			while(1){

				kLockForSpinLock( &(gs_vstScheduler[bCurrentAPICID].stSpinLock));
				pstTask = kRemoveListFromHeader(&(gs_vstScheduler[bCurrentAPICID].stWaitList));
				if(pstTask == NULL){
					kUnlockForSpinLock( &(gs_vstScheduler[bCurrentAPICID].stSpinLock));
					break;
				}

				// 프로세스일 경우 자식 스레드가 존재하면 모두 종료
				// 다시 자식 스레드 리스트에 삽입
				if(pstTask->qwFlags & TASK_FLAGS_PROCESS){
					iCount = kGetListCount(&(pstTask->stChildThreadList));

					for(i = 0; i < iCount; i++){
						// 임계 영역 시작
						kLockForSpinLock( &(gs_vstScheduler[bCurrentAPICID].stSpinLock));

						pvThreadLink = (TCB*)kRemoveListFromHeader(&(pstTask->stChildThreadList));
						if(pvThreadLink == NULL){
							kUnlockForSpinLock( &(gs_vstScheduler[bCurrentAPICID].stSpinLock));
							break;
						}

						// 자식 스레드 리스트에 연결된 정보는 태스크 자료구조에 있는
						// stThreadLink의 시작 어드레스이므로, 태스크 자료구조의 시작
						// 어드레스를 구하려면 별도의 계산이 필요함
						pstChildThread = GETTCBFROMTHREADLINK(pvThreadLink);

						// 다시 자식 스레드 리스트에 삽입하여 해당 스레드가 종료될 때
						// 자식 스레드가 프로세스를 찾아 스스로 리스트 제거하도록 함
						kAddListToTail(&(pstTask->stChildThreadList), &(pstChildThread->stThreadLink));
						qwChildThreadID = pstChildThread->stLink.qwID;

						kUnlockForSpinLock( &(gs_vstScheduler[bCurrentAPICID].stSpinLock));

						// 자식 스레드를 찾아서 종료
						kEndTask(qwChildThreadID);
					}

					// 아직 자식스레드가 남아 있다면 자식 스레드가 다 종료될 때까지
					// 기다려야하므로 다시 대기리스트에 삽입
					if(kGetListCount(&(pstTask->stChildThreadList)) > 0){
						kLockForSpinLock( &(gs_vstScheduler[bCurrentAPICID].stSpinLock));

						kAddListToTail(&(gs_vstScheduler[bCurrentAPICID].stWaitList), pstTask);

						kUnlockForSpinLock( &(gs_vstScheduler[bCurrentAPICID].stSpinLock));

						continue;

					// 프로세스를 종료해야하 하므로 할당받은 메모리 영역을 삭제
					}else{
						// TODO: ÃßÈÄ¿¡ ÄÚµå »ðÀÔ
					}

				// 스레드라면 프로세스의 자식 스레드 리스트에서 제거
				}else if(pstTask->qwFlags & TASK_FLAGS_THREAD){
					pstProcess = kGetProcessByThread(pstTask);

					if(pstProcess != NULL){

						// 프로세스 ID로 프로세스가 속한 스케줄러의 ID를 찾고 스핀락 잠금
						if(kFindSchedulerOfTaskAndLock(pstProcess->stLink.qwID, &bProcessAPICID) == TRUE){
							kRemoveList( &(pstProcess->stChildThreadList), pstTask->stLink.qwID);
							kUnlockForSpinLock( &(gs_vstScheduler[bCurrentAPICID].stSpinLock));
						}
					}
				}

				// 여기까지오면 태스크가 정상 종료된 것이므로 태스크 자료구조를 반환
				qwTaskID = pstTask->stLink.qwID;
				kFreeTCB(qwTaskID);
				kPrintf("IDLE: Task ID[0x%q] is completely ended.\n", qwTaskID);
			}
		}

		kSchedule();
	}
}

// 측정된 부하에 따라 프로세서를 쉬게함
void kHaltProcessorByLoad(BYTE bAPICID){
	if(gs_vstScheduler[bAPICID].qwProcessorLoad < 40){
		kHlt();
		kHlt();
		kHlt();

	}else if(gs_vstScheduler[bAPICID].qwProcessorLoad < 80){
		kHlt();
		kHlt();

	}else if(gs_vstScheduler[bAPICID].qwProcessorLoad < 95){
		kHlt();
	}
}

//====================================================================================================
// FPU 관련
//====================================================================================================
QWORD kGetLastFPUUsedTaskID(BYTE bAPICID){
	return gs_vstScheduler[bAPICID].qwLastFPUUsedTaskID;
}

void kSetLastFPUUsedTaskID(BYTE bAPICID, QWORD qwTaskID){
	gs_vstScheduler[bAPICID].qwLastFPUUsedTaskID = qwTaskID;
}
