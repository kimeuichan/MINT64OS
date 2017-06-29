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

// /***** 구조체 정의 *****/
#pragma pack(push, 1)

typedef struct kContextStruct{
	QWORD vqwRegister[TASK_REGISTERCOUNT];
} CONTEXT;

typedef struct kTaskControlBlockStruct{
	LISTLINK stLink;


	CONTEXT stContext;
	QWORD qwID;
	QWORD qwFlags;
	void* pvStackAddress;
	QWORD qwStackSize;
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

	LIST stReadyList;
} SCHEDULER;

#pragma pack(pop)

/***** 함수 정의 *****/
void kInitiailizeTCBPool(void);
TCB* kAllocateTCB(void);
void kFreeTCB(QWORD qwID);
TCB* kCreateTask(QWORD qwFlags, QWORD qwEntryPointAddress);
void kSetUpTask(TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress, void* pvStackAddress, QWORD qwStackSize);

// 스캐줄러 관련
void kInitializeScheduler(void);
void kSetRunningTask(TCB* pstTask);
TCB* kGetRunningTask(void);
TCB* kGetNextTaskToRun(void);
void kAddTaskToReadyList(TCB* pstTask);
void kSchedule(void);
BOOL kScheduleInInterrupt(void);
void kDecreaseProcessorTime(void);
BOOL kIsProcessorTimeExpired(void);

#endif // __TASK_H__