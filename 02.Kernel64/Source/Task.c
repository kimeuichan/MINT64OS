#include "Task.h"
#include "Descriptor.h"
#include "Utility.h"
#include "AssemblyUtility.h"
#include "Synchronization.h"
#include "Console.h"

/***** ���� ���� ���� *****/
static SCHEDULER gs_stScheduler;
static TCBPOOLMANAGER gs_stTCBPoolManager;

//====================================================================================================
// �½�ũ ���� �Լ�
//====================================================================================================
static void kInitializeTCBPool(void){
	int i;

	// TCB Ǯ �Ŵ��� �ʱ�ȭ
	kMemSet(&gs_stTCBPoolManager, 0, sizeof(gs_stTCBPoolManager));

	// TCB Ǯ ���� �ʱ�ȭ
	gs_stTCBPoolManager.pstStartAddress = (TCB*)TASK_TCBPOOLADDRESS;
	kMemSet((void*)TASK_TCBPOOLADDRESS, 0, sizeof(TCB) * TASK_MAXCOUNT);

	// TCB�� ID(������) �Ҵ�
	for(i = 0; i < TASK_MAXCOUNT; i++){
		gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;
	}

	// TCB�� �ִ� ������ �Ҵ� Ƚ�� �ʱ�ȭ
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
		// TCB ID(64��Ʈ)�� ���� 32��Ʈ�� 0�̸� �Ҵ���� ���� TCB
		if((gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID >> 32) == 0){
			pstEmptyTCB = &(gs_stTCBPoolManager.pstStartAddress[i]);
			break;
		}
	}

	// �Ҵ��� TCB ID�� ���� 32��Ʈ�� 0�� �ƴ� ������ ���� (TCB ID -> ���� 32��Ʈ:TCB �Ҵ� Ƚ��, ���� 32��Ʈ: TCB ������)
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

	// TCB ID�� ���� 32��Ʈ(TCB ������) ����
	i = GETTCBOFFSET(qwID);

	// ���ؽ�Ʈ�� TCB �Ҵ� Ƚ���� 0���� �ʱ�ȭ
	kMemSet(&(gs_stTCBPoolManager.pstStartAddress[i].stContext), 0, sizeof(CONTEXT));
	gs_stTCBPoolManager.pstStartAddress[i].stLink.qwID = i;

	gs_stTCBPoolManager.iUseCount--;
}

TCB* kCreateTask(QWORD qwFlags, void* pvMemoryAddress, QWORD qwMemorySize, QWORD qwEntryPointAddress){
	TCB* pstTask;    // ������ �½�ũ(���μ��� �Ǵ� ������)
	TCB* pstProcess; // ���� �½�ũ�� ���� ���μ���(�ڽ��� ������ ���μ���, �θ� ���μ���)
	void* pvStackAddress;
	BOOL bPreviousFlag;

	bPreviousFlag = kLockForSystemData();

	// �½�ũ �Ҵ�
	pstTask = kAllocateTCB();

	if(pstTask == NULL){
		kUnlockForSystemData(bPreviousFlag);
		return NULL;
	}

	// ���� �½�ũ�� ���� ���μ���(�ڽ��� ������ ���μ���, �θ� ���μ���) ���
	pstProcess = kGetProcessByThread(kGetRunningTask());

	if(pstProcess == NULL){
		kFreeTCB(pstTask->stLink.qwID);
		kUnlockForSystemData(bPreviousFlag);
		return NULL;
	}

	//****************************************************************************************************
	// �½�ũ ���� ����Ʈ�� ����ȭ
	// 1. �½�ũ ������
	//   -------------------
	//   |        -------- |  -> �ٱ��� �簢���� �غ� ����Ʈ(Ready List)
	//   |P1->P2->|T3=>T4| |  -> ���� �簢���� �ڽ� ������ ����Ʈ(Child Thread List)
	//   |        -------- |
	//   -------------------
	// 2. P2 ���μ��� �����
	//   ------------
	//   |T3->T4->P2|  -> ��� ����Ʈ(Wait List)
	//   ------------
	// 3. T3 ������ �����
	//   ----
	//   |T3|  -> ��� ����Ʈ(Wait List)
	//   ----
	//****************************************************************************************************

	// �����带 �����ϴ� ���
	if(qwFlags & TASK_FLAGS_THREAD){

		// [������.�θ� ���μ��� ID]�� �ڽ��� ������ ���μ���(�θ� ���μ���)�� ID�� �����ϰ�, [������.�޸� ����]�� �ڽ��� ������ ���μ���(�θ� ���μ���)�� �޸� ������ ������
		pstTask->qwParentProcessID = pstProcess->stLink.qwID;
		pstTask->pvMemoryAddress = pstProcess->pvMemoryAddress;
		pstTask->qwMemorySize = pstProcess->qwMemorySize;

		// �ڽ��� ������ ���μ���(�θ� ���μ���)�� �ڽ� ������ ����Ʈ�� ������ �����带 �߰�
		kAddListToTail(&(pstProcess->stChildThreadList), &(pstTask->stThreadLink));

	// ���μ����� �����ϴ� ���
	}else{
		// [���μ���.�θ� ���μ��� ID]�� �ڽ��� ������ ���μ���(�θ� ���μ���)�� ID�� �����ϰ�, [���μ���.�޸� ����]�� �Ķ���ͷ� �Ѿ� �� �޸� ������ ������
		pstTask->qwParentProcessID = pstProcess->stLink.qwID;
		pstTask->pvMemoryAddress = pvMemoryAddress;
		pstTask->qwMemorySize = qwMemorySize;
	}

	// ������ ID�� �½�ũ ID�� �����ϰ� ����
	pstTask->stThreadLink.qwID = pstTask->stLink.qwID;

	kUnlockForSystemData(bPreviousFlag);

	// �½�ũ�� ���� ��巹�� ���� (TCB ID�� �����¸� ���� Ǯ�� ���������� �̿�)
	pvStackAddress = (void*)(TASK_STACKPOOLADDRESS + (TASK_STACKSIZE * GETTCBOFFSET(pstTask->stLink.qwID)));

	// �½�ũ ����
	kSetUpTask(pstTask, qwFlags, qwEntryPointAddress, pvStackAddress, TASK_STACKSIZE);

	// �ڽ� ������ ����Ʈ �ʱ�ȭ
	kInitializeList(&(pstTask->stChildThreadList));

	// FPU ��� ���� �ʱ�ȭ
	pstTask->bFPUUsed = FALSE;

	bPreviousFlag = kLockForSystemData();

	// �½�ũ�� �غ� ����Ʈ�� �߰��Ͽ�, �����ٸ��� �� �ֵ��� ��
	kAddTaskToReadyList(pstTask);

	kUnlockForSystemData(bPreviousFlag);

	return pstTask;
}

static void kSetUpTask(TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress, void* pvStackAddress, QWORD qwStackSize){

	// ���ؽ�Ʈ �ʱ�ȭ
	kMemSet(pstTCB->stContext.vqwRegister, 0, sizeof(pstTCB->stContext.vqwRegister));

	// RSP, RBP ����
	pstTCB->stContext.vqwRegister[TASK_RSP_OFFSET] = (QWORD)pvStackAddress + qwStackSize - 8;
	pstTCB->stContext.vqwRegister[TASK_RBP_OFFSET] = (QWORD)pvStackAddress + qwStackSize - 8;

	// ������ �ֻ��� 8����Ʈ ������ ���� �ּҷμ�  kExitTask()�Լ��� ��巹���� ���������ν�,
	// �½�ũ�� ��Ʈ�� ����Ʈ �Լ��� return �Ǵ� ����, kExitTask()�Լ��� �̵��ϰ� ��
	*(QWORD*)((QWORD)pvStackAddress + qwStackSize - 8) = (QWORD)kExitTask;

	// ���׸�Ʈ ������ ����
	pstTCB->stContext.vqwRegister[TASK_CS_OFFSET] = GDT_KERNELCODESEGMENT;
	pstTCB->stContext.vqwRegister[TASK_DS_OFFSET] = GDT_KERNELDATASEGMENT;
	pstTCB->stContext.vqwRegister[TASK_ES_OFFSET] = GDT_KERNELDATASEGMENT;
	pstTCB->stContext.vqwRegister[TASK_FS_OFFSET] = GDT_KERNELDATASEGMENT;
	pstTCB->stContext.vqwRegister[TASK_GS_OFFSET] = GDT_KERNELDATASEGMENT;
	pstTCB->stContext.vqwRegister[TASK_SS_OFFSET] = GDT_KERNELDATASEGMENT;

	// RIP, RFLAGS ����
	pstTCB->stContext.vqwRegister[TASK_RIP_OFFSET] = qwEntryPointAddress;
	pstTCB->stContext.vqwRegister[TASK_RFLAGS_OFFSET] |= 0x0200; // RFLAGS�� IF(��Ʈ 9)=1�� �����Ͽ�, ���ͷ�Ʈ Ȱ��ȭ

	// ��Ÿ ����
	pstTCB->pvStackAddress = pvStackAddress;
	pstTCB->qwStackSize = qwStackSize;
	pstTCB->qwFlags = qwFlags;
}

//====================================================================================================
// �����ٷ� ���� �Լ�
//====================================================================================================
void kInitializeScheduler(void){
	int i;
	TCB* pstTask;

	// TCB Ǯ �ʱ�ȭ
	kInitializeTCBPool();

	for(i = 0; i < TASK_MAXREADYLISTCOUNT; i++){
		// �غ� ����Ʈ �ʱ�ȭ
		kInitializeList(&(gs_stScheduler.vstReadyList[i]));

		// �켱������ �½�ũ ���� Ƚ�� �ʱ�ȭ
		gs_stScheduler.viExecuteCount[i] = 0;
	}

	// ��� ����Ʈ �ʱ�ȭ
	kInitializeList(&(gs_stScheduler.stWaitList));

	// TCB�� �Ҵ�޾� �������� �½�ũ�� �����Ͽ�, ������ ������ �½�ũ(�� ���ؽ�Ʈ)�� ������ TCB�� �غ�
	// (������ ������ �½�ũ�� Ŀ�� ���� ���μ����� ��)
	pstTask = kAllocateTCB();
	gs_stScheduler.pstRunningTask = pstTask;
	pstTask->qwFlags = TASK_FLAGS_HIGHEST | TASK_FLAGS_PROCESS | TASK_FLAGS_SYSTEM; // ������ ������ �½�ũ(���Ŀ�  �ܼ� �� �½�ũ�� ��)�� �켱������ ���� �������� ����
	pstTask->qwParentProcessID = pstTask->stLink.qwID; // Ŀ�� ���� ���μ����� �θ� ���μ����� ���� ���� ��������, �θ� ���μ��� ID�� �ڽ��� ID�� ����
	pstTask->pvMemoryAddress = (void*)0x100000;
	pstTask->qwMemorySize = 0x500000;
	pstTask->pvStackAddress = (void*)0x600000;
	pstTask->qwStackSize = 0x100000;

	// ���μ��� ���� ���� ���� �ʱ�ȭ
	gs_stScheduler.qwSpendProcessorTimeInIdleTask = 0;
	gs_stScheduler.qwProcessorLoad = 0;

	// ������ FPU ��� �½�ũ ID �ʱ�ȭ
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

	// ��� �غ� ����Ʈ�� �½�ũ�� 1ȸ�� ����� ���, ��� �غ� ����Ʈ�� ���μ����� �纸�Ͽ�
	// �غ� ����Ʈ�� �½�ũ�� �������� �ұ��ϰ�, �½�ũ�� �������� �� �� �� ������
	// �½�ũ�� NULL�� ���, �ѹ� �� ����
	for(j = 0; j < 2; j++){
		// ���� �켱�������� ���� �켱�������� �غ� ����Ʈ�� Ȯ���Ͽ�, ������ ������ �½�ũ�� ����
		for(i = 0; i < TASK_MAXREADYLISTCOUNT; i++){
			// �켱������ �½�ũ ���� ���
			iTaskCount = kGetListCount(&(gs_stScheduler.vstReadyList[i]));

			// �ش� �غ� ����Ʈ�� �½�ũ ���� Ƚ������ �½�ũ ������ ���� ���, ���� �켱������ �½�ũ�� ����
			if(gs_stScheduler.viExecuteCount[i] < iTaskCount){
				pstTarget = (TCB*)kRemoveListFromHead(&(gs_stScheduler.vstReadyList[i]));
				gs_stScheduler.viExecuteCount[i]++;
				break;

			// �ش� �غ� ����Ʈ�� �½�ũ ���� Ƚ���� �½�ũ ������ ���ų� ���� ���, ���� �켱������ �½�ũ�� �纸
			}else{
				gs_stScheduler.viExecuteCount[i] = 0;
			}

		}

		// ������ ������ �½�ũ�� ������ ���, ���� Ż��
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

	// �½�ũ ID�� ��ȿ���� Ȯ��
	if(GETTCBOFFSET(qwTaskID) >= TASK_MAXCOUNT){
		return NULL;
	}

	// �½�ũ Ǯ���� �ش� �½�ũ�� ã�� ������ ID�� ��ġ�ϴ��� Ȯ��
	pstTarget = &(gs_stTCBPoolManager.pstStartAddress[GETTCBOFFSET(qwTaskID)]);
	if(pstTarget->stLink.qwID != qwTaskID){
		return NULL;
	}

	bPriority = GETPRIORITY(pstTarget->qwFlags);

	// �ش� �켱������ �غ� ����Ʈ���� �½�ũ ����
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

	// ���� �������� �½�ũ�� ���, �켱������ ����
	// Ÿ�̸� ���ͷ�Ʈ(IRQ0)�� �߻��Ͽ� �½�ũ ��ȯ�� ����� ��, ����� �켱������ �غ񸮽�Ʈ�� �̵���
	if(pstTarget->stLink.qwID == qwTaskID){
		SETPRIORITY(pstTarget->qwFlags, bPriority);

    // ���� �������� �½�ũ�� �ƴ� ���, �غ� ����Ʈ���� �����ϰ�, �켱������ ������ ��, ����� �켱������ �غ񸮽�Ʈ�� �̵���Ŵ
	}else{
		pstTarget = kRemoveTaskFromReadyList(qwTaskID);

		// �غ� ����Ʈ�� ���� ���, �½�ũ Ǯ���� ã�Ƽ� ���� �켱���� ����
		if(pstTarget == NULL){
			pstTarget = kGetTCBInTCBPool(GETTCBOFFSET(qwTaskID));
			if(pstTarget != NULL){
				SETPRIORITY(pstTarget->qwFlags, bPriority);
			}
		// �غ� ����Ʈ�� �ִ� ���, �켱������ ������ ��, ����� �켱������ �غ񸮽�Ʈ�� �̵���Ŵ
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
	// �½�ũ ��ȯ(�½�ũ �����)
	// - ���ؽ�Ʈ ���� : ��������->���� �½�ũ�� ���ؽ�Ʈ �޸� (kSwitchContext�� ó��)
	// - ���ؽ�Ʈ ���� : �ؽ�Ʈ �½�ũ�� ���ؽ�Ʈ �޸�->�������� (kSwitchContext�� ó��)
	//****************************************************************************************************

	pstRunningTask = gs_stScheduler.pstRunningTask;
	gs_stScheduler.pstRunningTask = pstNextTask;

	// ���� �½�ũ���� ��ȯ�� ���, ���� �½�ũ�� ����� ���μ��� �ð��� ������Ŵ
	if((pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE){
		gs_stScheduler.qwSpendProcessorTimeInIdleTask += (TASK_PROCESSORTIME - gs_stScheduler.iProcessorTime);
	}

	// ���� �½�ũ�� ������ FPU ��� �½�ũ�� �ƴ� ���, CR0.TS=1 �� ����
	if(gs_stScheduler.qwLastFPUUsedTaskID != pstNextTask->stLink.qwID){
		kSetTS();

	}else{
		kClearTS();
	}

	// ���� �½�ũ���� ��ȯ�� ���, ���� �½�ũ�� ��� ����Ʈ�� �̵���Ű��, �½�ũ ��ȯ ����
	if((pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK) == TASK_FLAGS_ENDTASK){
		kAddListToTail(&(gs_stScheduler.stWaitList), pstRunningTask);
		kSwitchContext(NULL, &(pstNextTask->stContext));

	// �Ϲ� �½�ũ���� ��ȯ�� ���, �Ϲ� �½�ũ�� �غ� ����Ʈ�� �̵���Ű��, �½�ũ ��ȯ ����
	}else{
		kAddTaskToReadyList(pstRunningTask);
		kSwitchContext(&(pstRunningTask->stContext), &(pstNextTask->stContext));
	}

	// ���μ��� ���ð� ����
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
	// �½�ũ ��ȯ(���ͷ�Ʈ �ڵ鷯 �����)
	// - ���ؽ�Ʈ ���� : ��������->IST�� ���ؽ�Ʈ �޸�(���μ���, ISR�� ó��)
	//             IST�� ���ؽ�Ʈ �޸�->���� �½�ũ�� ���ؽ�Ʈ �޸�(kMemCpy�� ó��)
	// - ���ؽ�Ʈ ���� : �ؽ�Ʈ �½�ũ�� ���ؽ�Ʈ �޸�->IST�� ���ؽ�Ʈ �޸�(kMemCpy�� ó��)
	//             IST�� ���ؽ�Ʈ �޸�->��������(���μ���, ISR�� ó��)
	//****************************************************************************************************

	pcContextAddress = (char*)IST_STARTADDRESS + IST_SIZE - sizeof(CONTEXT);
	pstRunningTask = gs_stScheduler.pstRunningTask;
	gs_stScheduler.pstRunningTask = pstNextTask;

	// ���� �½�ũ���� ��ȯ�� ���, ���� �½�ũ�� ����� ���μ��� �ð��� ������Ŵ
	if((pstRunningTask->qwFlags & TASK_FLAGS_IDLE) == TASK_FLAGS_IDLE){
		gs_stScheduler.qwSpendProcessorTimeInIdleTask += TASK_PROCESSORTIME;
	}

	// ���� �½�ũ���� ��ȯ�� ���, ���� �½�ũ�� ��� ����Ʈ�� �̵���Ű��, �½�ũ ��ȯ ����
	if((pstRunningTask->qwFlags & TASK_FLAGS_ENDTASK) == TASK_FLAGS_ENDTASK){
		kAddListToTail(&(gs_stScheduler.stWaitList), pstRunningTask);

	// �Ϲ� �½�ũ���� ��ȯ�� ���, �Ϲ� �½�ũ�� �غ� ����Ʈ�� �̵���Ű��, �½�ũ ��ȯ ����
	}else{
		kMemCpy(&(pstRunningTask->stContext), pcContextAddress, sizeof(CONTEXT));
		kAddTaskToReadyList(pstRunningTask);
	}

	kUnlockForSystemData(bPreviousFlag);

	// ���� �½�ũ�� ������ FPU ��� �½�ũ�� �ƴ� ���, CR0.TS=1 �� ����
	if(gs_stScheduler.qwLastFPUUsedTaskID != pstNextTask->stLink.qwID){
		kSetTS();

	}else{
		kClearTS();
	}

	kMemCpy(pcContextAddress, &(pstNextTask->stContext), sizeof(CONTEXT));

	// ���μ��� ���ð� ����
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

	// ���� �������� �½�ũ�� ���, ���� �½�ũ ��Ʈ�� �����ϰ�, �½�ũ ��ȯ ����
	if(pstTarget->stLink.qwID == qwTaskID){
		pstTarget->qwFlags |= TASK_FLAGS_ENDTASK;
		SETPRIORITY(pstTarget->qwFlags, TASK_FLAGS_WAIT);

		kUnlockForSystemData(bPreviousFlag);

		// �½�ũ ��ȯ
		kSchedule();

		// �½�ũ�� ��ȯ�Ǿ����Ƿ� �Ʒ� �ڵ�� ������� ����
		while(1);

	// ���� �������� �½�ũ�� �ƴ� ���, �غ� ����Ʈ���� �����ϰ�, ���� �½�ũ ��Ʈ�� ������ ��, ��� ����Ʈ�� �̵���Ŵ
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

	//�� �½�ũ �� = ���� �غ� �½�ũ �� + ���� ��� �½�ũ �� + ���� �������� �½�ũ ��
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

	// �Ķ���Ͱ� ���μ����� ���, �ڽ��� ����
	if(pstThread->qwFlags & TASK_FLAGS_PROCESS){
		return pstThread;
	}

	// �Ķ���Ͱ� �������� ���, �θ� ���μ����� ����
	pstProcess = kGetTCBInTCBPool(GETTCBOFFSET(pstThread->qwParentProcessID));
	if((pstProcess == NULL) || (pstProcess->stLink.qwID != pstThread->qwParentProcessID)){
		return NULL;
	}

	return pstProcess;
}

//====================================================================================================
// ���� �½�ũ ���� �Լ�
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

	// ���� ����
	while(1){

		//----------------------------------------------------------------------------------------------------
		// 1. ���μ��� ���� ���
		//----------------------------------------------------------------------------------------------------

		qwCurrentMeasureTickCount = kGetTickCount();
		qwCurrentSpendTickInIdleTask = gs_stScheduler.qwSpendProcessorTimeInIdleTask;

		// ���μ��� ����(%) = 100 - (���� �½�ũ�� ����� ���μ��� �ð� * 100 / �ý��� ��ü���� ����� ���μ��� �ð�)
		if((qwCurrentMeasureTickCount - qwLastMeasureTickCount) == 0){
			gs_stScheduler.qwProcessorLoad = 0;

		}else{
			gs_stScheduler.qwProcessorLoad = 100 - ((qwCurrentSpendTickInIdleTask - qwLastSpendTickInIdleTask) * 100 / (qwCurrentMeasureTickCount - qwLastMeasureTickCount));
		}

		qwLastMeasureTickCount = qwCurrentMeasureTickCount;
		qwLastSpendTickInIdleTask = qwCurrentSpendTickInIdleTask;

		//----------------------------------------------------------------------------------------------------
		// 2. ���μ��� ������ ���� ���μ����� ���� ��
		//----------------------------------------------------------------------------------------------------

		// ���μ��� ����(���μ��� ����)�� ���� ���μ����� ���� ��
		kHaltProcessorByLoad();

		//----------------------------------------------------------------------------------------------------
		// 3. ��� ����Ʈ�� �½�ũ�� �����Ŵ
		//----------------------------------------------------------------------------------------------------

		// ��� ����Ʈ�� ���� �½�ũ�� �ִ� ���, ��� ����Ʈ���� ���� �½�ũ�� �����ϰ�, ���� �½�ũ�� �޸𸮸� ������
		if(kGetListCount(&(gs_stScheduler.stWaitList)) > 0){
			while(1){

				bPreviousFlag = kLockForSystemData();

				// ��� ����Ʈ���� ���� �½�ũ�� ����
				pstTask = kRemoveListFromHead(&(gs_stScheduler.stWaitList));
				if(pstTask == NULL){
					kUnlockForSystemData(bPreviousFlag);
					break;
				}

				// [�ڵ� ��� 1]���� �½�ũ�� ���μ����� ���, ���μ����� �ڽ� �����带 ��� ����(�غ� ����Ʈ->��� ����Ʈ)��Ų ��,
				//          ���� �½�ũ�� ��� ����Ʈ�� �ڽ� �����带 ��� ������ ����(�޸� ����)��ų ������ ��� �� �� �������� ���μ��� �ڽ��� ������ ����(�޸� ����)��Ŵ
				// [����] ���μ����� �޸� ���� : �ڵ�/������ ����, TCB ����, ���� ����
				//      �������� �޸� ����     : TCB ����, ���� ����
				if(pstTask->qwFlags & TASK_FLAGS_PROCESS){
					iCount = kGetListCount(&(pstTask->stChildThreadList));

					for(i = 0; i < iCount; i++){
						pvThreadLink = (TCB*)kRemoveListFromHead(&(pstTask->stChildThreadList));
						if(pvThreadLink == NULL){
							break;
						}

						pstChildThread = GETTCBFROMTHREADLINK(pvThreadLink);

						// [����]�ڽ� �����带 �ڽ� ������ ����Ʈ���� ������ ��, �ٽ� �����ϴ� ����
						//      ù��° ���� : �����带 �����Ű�� ����� ������� [�ڵ� ��� 2]���� �ڽ��� ������ �ڽ� ������ ����Ʈ���� ������
						//      �ι�° ���� : ��Ƽ �ھ� ���μ��� ȯ���� ���, �ڽ� �����尡 �ٸ� �ھ�� ���� ���� �� ������, �̷� ��� �ڽ� �����带 ��� ��� ����Ʈ�� �̵���ų �� ����
						kAddListToTail(&(pstTask->stChildThreadList), &(pstChildThread->stThreadLink));

						// �ڽ� �����带 ��� ����(�غ� ����Ʈ->��� ����Ʈ)��Ŵ
						kEndTask(pstChildThread->stLink.qwID);
					}

					// �ڽ� �����尡 ���� �ִ� ���, ���� �½�ũ�� ��� ����Ʈ�� �ڽ� �����带 ��� ������ ����(�޸� ����)��ų ������ ��� ��
					if(kGetListCount(&(pstTask->stChildThreadList)) > 0){
						kAddListToTail(&(gs_stScheduler.stWaitList), pstTask);

						kUnlockForSystemData(bPreviousFlag);
						continue;

					// �ڽ� �����尡 ��� ������ ����(�޸� ����)�� ���, ���μ��� �ڽ��� ������ ����(�޸� ����)��Ŵ
					}else{
						// TODO: ���Ŀ� �ڵ� ����
						// ���� ���μ����� �ڵ�/������ ���� �޸𸮸� ����
					}

				// [�ڵ� ��� 2]���� �½�ũ�� �������� ���, �����尡 ���� ���μ����� �ڽ� ������ ����Ʈ���� �ڽ��� �����ϰ�, ������ �ڽ��� ������ ����(�޸� ����)��Ŵ
				}else if(pstTask->qwFlags & TASK_FLAGS_THREAD){
					pstProcess = kGetProcessByThread(pstTask);

					if(pstProcess != NULL){
						kRemoveList(&(pstProcess->stChildThreadList), pstTask->stLink.qwID);
					}
				}

				// ���� �½�ũ�� TCB ���� �޸𸮸� ����(TCB ������ �����ϸ�, ���� ������ �ڵ����� ������)
				qwTaskID = pstTask->stLink.qwID;
				kFreeTCB(qwTaskID);

				kUnlockForSystemData(bPreviousFlag);

				kPrintf("IDLE: Task ID[0x%q] is completely ended.\n", qwTaskID);
			}
		}

		// �½�ũ ��ȯ
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
// FPU ���� �Լ�
//====================================================================================================
QWORD kGetLastFPUUsedTaskID(void){
	return gs_stScheduler.qwLastFPUUsedTaskID;
}

void kSetLastFPUUsedTaskID(QWORD qwTaskID){
	gs_stScheduler.qwLastFPUUsedTaskID = qwTaskID;
}
