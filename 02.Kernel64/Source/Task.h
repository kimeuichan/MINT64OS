#ifndef __TASK_H__
#define __TASK_H__

#include "Types.h"
#include "List.h"

/***** 매크로 정의 *****/
// 콘텍스트 레지스터 개수와 크기
#define TASK_REGISTERCOUNT 24 // 24 = 5(SS, RSP, RFLAGS, CS, RIP) + 19(ISR에서 저장하는 레지스터들)
#define TASK_REGISTERSIZE  8

//CONTEXT 자료구조의 레지스터 오프셋
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

// 태스크 풀의 어드레스
#define TASK_TCBPOOLADDRESS	0x800000
#define TASK_MAXCOUNT	1024

// ﾽﾺￅￃ ￇﾮ ﾰ￼ﾷￃ ﾸￅￅﾩﾷￎ
#define TASK_STACKPOOLADDRESS (TASK_TCBPOOLADDRESS + (sizeof(TCB) * TASK_MAXCOUNT))
#define TASK_STACKSIZE        8192 // 8KB

// 유효하지 않은 태스크 ID
#define TASK_INVALIDID 0xffffffffffffffff

// 태스크가 최대로 쓸 수 있는 프로세서 시간(5ms)
#define TASK_PROCESSORTIME	5

// 준비 리스트의 수
#define TASK_MAXREADYLISTCOUNT	5

// 태스크의 우선순위
#define TASK_FLAGS_HIGHEST	0
#define TASK_FLAGS_HIGH	1
#define TASK_FLAGS_MEDIUM	2
#define TASK_FLAGS_LOW	3
#define TASK_FLAGS_LOWEST	4
#define TASK_FLAGS_WAIT	0xff

// 태스크의 플래그
#define TASK_FLAGS_ENDTASK		0x8000000000000000
#define TASK_FLAGS_SYSTEM		0x4000000000000000
#define TASK_FLAGS_PROCESS		0x2000000000000000
#define TASK_FLAGS_THREAD		0x1000000000000000
#define TASK_FLAGS_IDLE			0x0800000000000000

// 함수 매크로
#define GETPRIORITY(x)	((x)&0xff)
#define SETPRIORITY(x, priority)	((x) = ((x)&0xffffffffffffff00) | \
	(priority))
#define GETTCBOFFSET(x) 		((x)&0xffffffff)
// 자식 스레드 링크에 연결된 stThreadLink 정보에서 태스크 자료구조(TCB) 위치를
// 계산하여 반환하는 매크로
#define GETTCBFROMTHREADLINK(x)  (TCB*)((QWORD)(x) - offsetof(TCB, stThreadLink))

// /***** 구조체 정의 *****/
#pragma pack(push, 1)

typedef struct kContextStruct{
	QWORD vqwRegister[TASK_REGISTERCOUNT];
} CONTEXT;

// 태스크(프로세스 및 스레드)의 상태를 관리하는 자료 구조
typedef struct kTaskControlBlockStruct{
	LISTLINK stLink;


	QWORD qwID;
	QWORD qwFlags;

	// 프로세스 메모리 영역의 시작과 크기
	void* pvMemoryAddress;
	QWORD qwMemorySize;

	// 이하 스레드 정보
	// 스레드 간의 연결
	LISTLINK stThreadLink;
	// 자식 스레드의 리스트
	QWORD qwParentProcessID;

	// FPU 콘텍스트는 16배수로 정렬되어야 하므로, 앞으로 추가할 데이터는 현재 라인 아래에 추가해야함
	QWORD vqwFPUContext[512/8];

	LIST stChildThreadList;


	CONTEXT stContext;

	void* pvStackAddress;
	QWORD qwStackSize;
	BOOL bFPUUsed;
} TCB;

typedef struct kTCBPOOLManagerStruct{
	TCB* pstStartAddress;
	int iMaxCount;
	int iUseCount;

	//TCB 할당 갯수
	int iAllocatedCount;
} TCBPOOLMANAGER;

typedef struct kSchedulerStruct{
	TCB* pstRunningTask;

	int iProcessorTime;

	LIST vstReadyList[TASK_MAXREADYLISTCOUNT];

	LIST stWaitList;

	int viExecuteCount[TASK_MAXREADYLISTCOUNT];

	QWORD qwProcessorLoad;

	QWORD qwSpendProcessorTimeInIdleTask;

	QWORD qwLastFPUUsedTaskID;


} SCHEDULER;


#pragma pack(pop)

/***** 함수 정의 *****/
static void kInitiailizeTCBPool(void);
static TCB* kAllocateTCB(void);
static void kFreeTCB(QWORD qwID);
TCB* kCreateTask(QWORD qwFlags, void* pvMemoryAddress, QWORD qwMemorySize, QWORD qwEntryPointAddress);
static void kSetUpTask(TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress, void* pvStackAddress, QWORD qwStackSize);

// 스캐줄러 관련
void kInitializeScheduler(void);
void kSetRunningTask(TCB* pstTask);
TCB* kGetRunningTask(void);
static TCB* kGetNextTaskToRun(void);
static BOOL kAddTaskToReadyList(TCB* pstTask);
void kSchedule(void);
BOOL kScheduleInInterrupt(void);
void kDecreaseProcessorTime(void);
BOOL kIsProcessorTimeExpired(void);
static TCB* kRemoveTaskFromReadyList(QWORD qwTaskID);
BOOL kChangePriority(QWORD qwTaskID, BYTE bPriority);
BOOL kEndTask(QWORD qwTaskID);
void kExitTask(void);
int kGetReadyTaskCount(void);
int kGetTaskCount(void);
TCB* kGetTCBInTCBPool(int iOffset);
BOOL kIsTaskExist(QWORD qwID);
QWORD kGetProcessorLoad(void);
static TCB* kGetProcessByThread(TCB* pstThread);

// 유휴 태스크 관련
void kIdleTask(void);
void kHaltProcessorByLoad(void);

// FPU 관련
QWORD kGetLastFPUUsedTaskID(void);
void kSetLastFPUUsedTaskID(QWORD qwTaskID);

#endif // __TASK_H__