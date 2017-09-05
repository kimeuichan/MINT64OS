#ifndef __TASK_H__
#define __TASK_H__

#include "Types.h"
#include "List.h"

/***** ��ũ�� ���� *****/
// ���ؽ�Ʈ �������� ������ ũ��
#define TASK_REGISTERCOUNT 24 // 24 = 5(SS, RSP, RFLAGS, CS, RIP) + 19(ISR���� �����ϴ� �������͵�)
#define TASK_REGISTERSIZE  8

// CONTEXT �ڷᱸ���� �������� ������
#define TASK_GS_OFFSET     0
#define TASK_FS_OFFSET     1
#define TASK_ES_OFFSET     2
#define TASK_DS_OFFSET     3
#define TASK_R15_OFFSET    4
#define TASK_R14_OFFSET    5
#define TASK_R13_OFFSET    6
#define TASK_R12_OFFSET    7
#define TASK_R11_OFFSET    8
#define TASK_R10_OFFSET    9
#define TASK_R9_OFFSET     10
#define TASK_R8_OFFSET     11
#define TASK_RSI_OFFSET    12
#define TASK_RDI_OFFSET    13
#define TASK_RDX_OFFSET    14
#define TASK_RCX_OFFSET    15
#define TASK_RBX_OFFSET    16
#define TASK_RAX_OFFSET    17
#define TASK_RBP_OFFSET    18
#define TASK_RIP_OFFSET    19
#define TASK_CS_OFFSET     20
#define TASK_RFLAGS_OFFSET 21
#define TASK_RSP_OFFSET    22
#define TASK_SS_OFFSET     23

// �½�ũ Ǯ ���� ��ũ��
#define TASK_TCBPOOLADDRESS 0x800000 // 8M
#define TASK_MAXCOUNT       1024

// ���� Ǯ ���� ��ũ��
#define TASK_STACKPOOLADDRESS (TASK_TCBPOOLADDRESS + (sizeof(TCB) * TASK_MAXCOUNT))
#define TASK_STACKSIZE        8192 // 8KB

// ��ȿ���� ���� �½�ũ ID
#define TASK_INVALIDID     0xFFFFFFFFFFFFFFFF

// �½�ũ�� �ѹ��� �ִ�� �� �� �ִ� ���μ��� �ð�(5ms)
#define TASK_PROCESSORTIME 5

// �غ� ����Ʈ ����
#define TASK_MAXREADYLISTCOUNT 5

// �½�ũ �켱���� (�½�ũ �÷���(64��Ʈ)�� ���� 8��Ʈ )
#define TASK_FLAGS_HIGHEST 0    // ���� ����
#define TASK_FLAGS_HIGH    1    // ����
#define TASK_FLAGS_MEDIUM  2    // �߰�
#define TASK_FLAGS_LOW     3    // ����
#define TASK_FLAGS_LOWEST  4    // ���� ����
#define TASK_FLAGS_WAIT    0xFF // ���� �½�ũ �켱����

// �½�ũ �÷���
#define TASK_FLAGS_ENDTASK 0x8000000000000000 // ���� �½�ũ
#define TASK_FLAGS_SYSTEM  0x4000000000000000 // �ý��� �½�ũ
#define TASK_FLAGS_PROCESS 0x2000000000000000 // ���μ���
#define TASK_FLAGS_THREAD  0x1000000000000000 // ������
#define TASK_FLAGS_IDLE    0x0800000000000000 // ���� �½�ũ

// ��Ʈ�� �Լ�
#define GETPRIORITY(x)           ((x) & 0xFF)                                     // TCB.qwFlags(64��Ʈ)�� ���� 8��Ʈ ����
#define SETPRIORITY(x, priority) ((x) = ((x) & 0xFFFFFFFFFFFFFF00) | (priority))  // TCB.qwFlags(64��Ʈ)�� ���� 8��Ʈ ����
#define GETTCBOFFSET(x)          ((x) & 0xFFFFFFFF)                               // TCB.stLink.qwID(64��Ʈ)�� ���� 32��Ʈ ����
#define GETTCBFROMTHREADLINK(x)  (TCB*)((QWORD)(x) - offsetof(TCB, stThreadLink)) // TCB.stThreadLink ��巹���� ����, TCB ��巹���� ����Ͽ� ��ȯ��

/***** ����ü ���� *****/
#pragma pack(push, 1)

typedef struct kContextStruct{
	QWORD vqwRegister[TASK_REGISTERCOUNT];
} CONTEXT;

typedef struct kTaskControlBlockStruct{
	LISTLINK stLink;            // �����ٷ� ��ũ -> ���� �½�ũ ��ġ(stLink.pvNext)
	                            //         -> �½�ũ ID(stLink.qwID) -> ���� 32��Ʈ: TCB �Ҵ� Ƚ��
	                            //                                -> ���� 32��Ʈ: TCB ������
	                            //         [����:LISTLINK�� �ݵ�� ����ü�� �� �տ� ��ġ�ؾ� ��]
	QWORD qwFlags;              // �½�ũ �÷��� -> ��Ʈ 63: ���� �½�ũ ����
                                //         -> ��Ʈ 62: �ý��� �½�ũ ����
	                            //         -> ��Ʈ 61: ���μ��� ����
	                            //         -> ��Ʈ 60: ������ ����
	                            //         -> ��Ʈ 59: ���� �½�ũ ����
	                            //         -> ��Ʈ 7~0: �½�ũ �켱����
	void* pvMemoryAddress;      // ���μ��� �޸� ���� ���� ��巹��(�ڵ�, ������ ����)
	QWORD qwMemorySize;         // ���μ��� �޸� ���� ũ��(�ڵ�, ������ ����)
	//--------------------------------------------------
	// ���ϴ� ������ ����
	//--------------------------------------------------
	LISTLINK stThreadLink;      // �ڽ� ������ ��ũ(���� �ڽ� ������ ��ġ, ������ ID)
	QWORD qwParentProcessID;    // �θ� ���μ��� ID
	QWORD vqwFPUContext[512/8]; // FPU ���ؽ�Ʈ -> [����]FPU ���ؽ�Ʈ�� ���� ��巹���� �ݵ�� 16�� ���������. �׷��� ���ؼ��� �Ʒ� 3���� ������ �������Ѿ� ��.
	                            // (512 byte ����)   ���� 1: TCB Ǯ�� ���� ��巹���� 16�� ��������� (���� 0x800000(8MB))
	                            //                  ���� 2: �� TCB�� ũ�Ⱑ 16�� ��������� (���� 816B)
                                //                  ���� 3: �� TCB���� FPU ���ؽ�Ʈ�� �������� 16�� ��������� (���� 64B)
	                            //                  ���� �� 3���� ������ �����ϹǷ�, ���Ŀ� TCB�� �ʵ� �߰��� �ݵ�� vqwFPUContext�� �Ʒ��� �߰��ؾ� ��.
	LIST stChildThreadList;     // �ڽ� ������ ����Ʈ
	CONTEXT stContext;          // ���ؽ�Ʈ
	void* pvStackAddress;       // ���� ���� ��巹��
	QWORD qwStackSize;          // ���� ũ��
	BOOL bFPUUsed;              // FPU ��� ���� (�ش� �½�ũ���� ������ FPU ������ ����� ���� �ִ��� ����)
	char vcPadding[11];         // �е� ����Ʈ (FPU ���ؽ�Ʈ�� ����.���� 2�� ����, TCB�� ũ�⸦ 16 byte�� ����� ���߱� ����)
} TCB; // LISTITEM�� �ش�, ���� TCB ũ��� 816 byte��

typedef struct kTCBPoolManagerStruct{
	TCB* pstStartAddress; // TCB Ǯ ���� �ּ�
	int iMaxCount;        // TCB �ִ� ����(TCB Ǯ���� �ִ� TCB ����)
	int iUseCount;        // TCB ��� ����(TCB Ǯ���� ������� TCB ����)
	int iAllocatedCount;  // TCB �Ҵ� Ƚ��(TCB Ǯ���� TCB�� �Ҵ�� Ƚ��)
} TCBPOOLMANAGER;

typedef struct kSchedulerStruct{
	TCB* pstRunningTask;                        // ���� �������� �½�ũ
	int iProcessorTime;                         // ���� �������� �½�ũ�� ����� �� �ִ� ���μ��� �ð�
	LIST vstReadyList[TASK_MAXREADYLISTCOUNT];  // �غ� ����Ʈ(������ �½�ũ�� �غ����� ����Ʈ, �½�ũ �켱������ ���� ����)
	LIST stWaitList;                            // ��� ����Ʈ(������ �½�ũ�� ������� ����Ʈ)
	int viExecuteCount[TASK_MAXREADYLISTCOUNT]; // �켱������ �½�ũ ���� Ƚ��
	QWORD qwProcessorLoad;                      // ���μ��� ����(���μ��� ����)
	QWORD qwSpendProcessorTimeInIdleTask;       // ���� �½�ũ�� ����� ���μ��� �ð�
	QWORD qwLastFPUUsedTaskID;                  // ������ FPU ��� �½�ũ ID
} SCHEDULER;

#pragma pack(pop)

/***** �Լ� ���� *****/
// �½�ũ ���� �Լ�
static void kInitializeTCBPool(void);
static TCB* kAllocateTCB(void);
static void kFreeTCB(QWORD qwID);
TCB* kCreateTask(QWORD qwFlags, void* pvMemoryAddress, QWORD qwMemorySize, QWORD qwEntryPointAddress);
static void kSetUpTask(TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress, void* pvStackAddress, QWORD qwStackSize);

// �����ٷ� ���� �Լ�
void kInitializeScheduler(void);
void kSetRunningTask(TCB* pstTask);
TCB* kGetRunningTask(void);
static TCB* kGetNextTaskToRun(void);           // �غ� ����Ʈ���� ������ ������ �½�ũ ����(����)
static BOOL kAddTaskToReadyList(TCB* pstTask); // �غ� ����Ʈ�� �½�ũ �߰�
void kSchedule(void);                          // �½�ũ ��ȯ(�½�ũ �����)
BOOL kScheduleInInterrupt(void);               // �½�ũ ��ȯ(���ͷ�Ʈ �ڵ鷯 �����)
void kDecreaseProcessorTime(void);
BOOL kIsProcessorTimeExpired(void);
static TCB* kRemoveTaskFromReadyList(QWORD qwTaskID); // �غ� ����Ʈ���� �½�ũ ����
BOOL kChangePriority(QWORD qwTaskID, BYTE bPriority); // �½�ũ �켱���� ����
BOOL kEndTask(QWORD qwTaskID);                        // �½�ũ ����
void kExitTask(void);                                 // �½�ũ�� �������� �ڽ��� ����
int kGetReadyTaskCount(void); // ���� �غ� �½�ũ �� ��ȯ
int kGetTaskCount(void);      // �� �½�ũ �� ��ȯ(�� �½�ũ �� = ���� �غ� �½�ũ �� + ���� ��� �½�ũ �� + ���� �������� �½�ũ ��)
TCB* kGetTCBInTCBPool(int iOffset);
BOOL kIsTaskExist(QWORD qwTaskID);
QWORD kGetProcessorLoad(void);
static TCB* kGetProcessByThread(TCB* pstThread); // �ڽ��� ���� ���μ��� ���(���μ����� ��� �ڽ��� �����ϰ�, �������� ��� �θ� ���μ����� ������)

// ���� �½�ũ ���� �Լ�
void kIdleTask(void);
void kHaltProcessorByLoad(void);

// FPU ���� �Լ�
QWORD kGetLastFPUUsedTaskID(void);
void kSetLastFPUUsedTaskID(QWORD qwTaskID);

#endif // __TASK_H__
