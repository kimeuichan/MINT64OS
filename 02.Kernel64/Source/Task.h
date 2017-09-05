#ifndef __TASK_H__
#define __TASK_H__

#include "Types.h"
#include "List.h"

/***** 매크로 정의 *****/
// 콘텍스트 레지스터 개수와 크기
#define TASK_REGISTERCOUNT 24 // 24 = 5(SS, RSP, RFLAGS, CS, RIP) + 19(ISR에서 저장하는 레지스터들)
#define TASK_REGISTERSIZE  8

// CONTEXT 자료구조의 레지스터 오프셋
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

// 태스크 풀 관련 매크로
#define TASK_TCBPOOLADDRESS 0x800000 // 8M
#define TASK_MAXCOUNT       1024

// 스택 풀 관련 매크로
#define TASK_STACKPOOLADDRESS (TASK_TCBPOOLADDRESS + (sizeof(TCB) * TASK_MAXCOUNT))
#define TASK_STACKSIZE        8192 // 8KB

// 유효하지 않은 태스크 ID
#define TASK_INVALIDID     0xFFFFFFFFFFFFFFFF

// 태스크가 한번에 최대로 쓸 수 있는 프로세서 시간(5ms)
#define TASK_PROCESSORTIME 5

// 준비 리스트 개수
#define TASK_MAXREADYLISTCOUNT 5

// 태스크 우선순위 (태스크 플래그(64비트)의 하위 8비트 )
#define TASK_FLAGS_HIGHEST 0    // 가장 높음
#define TASK_FLAGS_HIGH    1    // 높음
#define TASK_FLAGS_MEDIUM  2    // 중간
#define TASK_FLAGS_LOW     3    // 낮음
#define TASK_FLAGS_LOWEST  4    // 가장 낮음
#define TASK_FLAGS_WAIT    0xFF // 종료 태스크 우선순위

// 태스크 플래그
#define TASK_FLAGS_ENDTASK 0x8000000000000000 // 종료 태스크
#define TASK_FLAGS_SYSTEM  0x4000000000000000 // 시스템 태스크
#define TASK_FLAGS_PROCESS 0x2000000000000000 // 프로세스
#define TASK_FLAGS_THREAD  0x1000000000000000 // 스레드
#define TASK_FLAGS_IDLE    0x0800000000000000 // 유휴 태스크

// 매트로 함수
#define GETPRIORITY(x)           ((x) & 0xFF)                                     // TCB.qwFlags(64비트)의 하위 8비트 추출
#define SETPRIORITY(x, priority) ((x) = ((x) & 0xFFFFFFFFFFFFFF00) | (priority))  // TCB.qwFlags(64비트)의 하위 8비트 설정
#define GETTCBOFFSET(x)          ((x) & 0xFFFFFFFF)                               // TCB.stLink.qwID(64비트)의 하위 32비트 추출
#define GETTCBFROMTHREADLINK(x)  (TCB*)((QWORD)(x) - offsetof(TCB, stThreadLink)) // TCB.stThreadLink 어드레스를 통해, TCB 어드레스를 계산하여 반환함

/***** 구조체 정의 *****/
#pragma pack(push, 1)

typedef struct kContextStruct{
	QWORD vqwRegister[TASK_REGISTERCOUNT];
} CONTEXT;

typedef struct kTaskControlBlockStruct{
	LISTLINK stLink;            // 스케줄러 링크 -> 다음 태스크 위치(stLink.pvNext)
	                            //         -> 태스크 ID(stLink.qwID) -> 상위 32비트: TCB 할당 횟수
	                            //                                -> 하위 32비트: TCB 오프셋
	                            //         [주의:LISTLINK가 반드시 구조체의 맨 앞에 위치해야 함]
	QWORD qwFlags;              // 태스크 플래그 -> 비트 63: 종료 태스크 여부
                                //         -> 비트 62: 시스템 태스크 여부
	                            //         -> 비트 61: 프로세서 여부
	                            //         -> 비트 60: 스레드 여부
	                            //         -> 비트 59: 유휴 태스크 여부
	                            //         -> 비트 7~0: 태스크 우선순위
	void* pvMemoryAddress;      // 프로세스 메모리 영역 시작 어드레스(코드, 데이터 영역)
	QWORD qwMemorySize;         // 프로세서 메모리 영역 크기(코드, 데이터 영역)
	//--------------------------------------------------
	// 이하는 스레드 정보
	//--------------------------------------------------
	LISTLINK stThreadLink;      // 자식 스레드 링크(다음 자식 스레드 위치, 스레드 ID)
	QWORD qwParentProcessID;    // 부모 프로세스 ID
	QWORD vqwFPUContext[512/8]; // FPU 콘텍스트 -> [주의]FPU 콘텍스트의 시작 어드레스는 반드시 16의 배수여야함. 그러기 위해서는 아래 3가지 조건을 만족시켜야 함.
	                            // (512 byte 고정)   조건 1: TCB 풀의 시작 어드레스가 16의 배수여야함 (현재 0x800000(8MB))
	                            //                  조건 2: 각 TCB의 크기가 16의 배수여야함 (현재 816B)
                                //                  조건 3: 각 TCB에서 FPU 콘텍스트의 오프셋이 16의 배수여야함 (현재 64B)
	                            //                  현재 위 3가지 조건을 만족하므로, 이후에 TCB에 필드 추가시 반드시 vqwFPUContext의 아래에 추가해야 함.
	LIST stChildThreadList;     // 자식 스레드 리스트
	CONTEXT stContext;          // 콘텍스트
	void* pvStackAddress;       // 스택 시작 어드레스
	QWORD qwStackSize;          // 스택 크기
	BOOL bFPUUsed;              // FPU 사용 여부 (해당 태스크에서 이전에 FPU 연산을 사용한 적이 있는지 여부)
	char vcPadding[11];         // 패딩 바이트 (FPU 콘텍스트의 주의.조건 2에 따라, TCB의 크기를 16 byte의 배수에 맞추기 위함)
} TCB; // LISTITEM에 해당, 현재 TCB 크기는 816 byte임

typedef struct kTCBPoolManagerStruct{
	TCB* pstStartAddress; // TCB 풀 시작 주소
	int iMaxCount;        // TCB 최대 개수(TCB 풀에서 최대 TCB 개수)
	int iUseCount;        // TCB 사용 개수(TCB 풀에서 사용중인 TCB 개수)
	int iAllocatedCount;  // TCB 할당 횟수(TCB 풀에서 TCB가 할당된 횟수)
} TCBPOOLMANAGER;

typedef struct kSchedulerStruct{
	TCB* pstRunningTask;                        // 현재 실행중인 태스크
	int iProcessorTime;                         // 현재 실행중인 태스크가 사용할 수 있는 프로세서 시간
	LIST vstReadyList[TASK_MAXREADYLISTCOUNT];  // 준비 리스트(실행할 태스크가 준비중인 리스트, 태스크 우선순위에 따라 구분)
	LIST stWaitList;                            // 대기 리스트(종료할 태스크가 대기중인 리스트)
	int viExecuteCount[TASK_MAXREADYLISTCOUNT]; // 우선순위별 태스크 실행 횟수
	QWORD qwProcessorLoad;                      // 프로세서 부하(프로세서 사용률)
	QWORD qwSpendProcessorTimeInIdleTask;       // 유휴 태스크가 사용한 프로세서 시간
	QWORD qwLastFPUUsedTaskID;                  // 마지막 FPU 사용 태스크 ID
} SCHEDULER;

#pragma pack(pop)

/***** 함수 정의 *****/
// 태스크 관련 함수
static void kInitializeTCBPool(void);
static TCB* kAllocateTCB(void);
static void kFreeTCB(QWORD qwID);
TCB* kCreateTask(QWORD qwFlags, void* pvMemoryAddress, QWORD qwMemorySize, QWORD qwEntryPointAddress);
static void kSetUpTask(TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress, void* pvStackAddress, QWORD qwStackSize);

// 스케줄러 관련 함수
void kInitializeScheduler(void);
void kSetRunningTask(TCB* pstTask);
TCB* kGetRunningTask(void);
static TCB* kGetNextTaskToRun(void);           // 준비 리스트에서 다음에 실행할 태스크 선택(삭제)
static BOOL kAddTaskToReadyList(TCB* pstTask); // 준비 리스트에 태스크 추가
void kSchedule(void);                          // 태스크 전환(태스크 수행시)
BOOL kScheduleInInterrupt(void);               // 태스크 전환(인터럽트 핸들러 수행시)
void kDecreaseProcessorTime(void);
BOOL kIsProcessorTimeExpired(void);
static TCB* kRemoveTaskFromReadyList(QWORD qwTaskID); // 준비 리스트에서 태스크 삭제
BOOL kChangePriority(QWORD qwTaskID, BYTE bPriority); // 태스크 우선순위 변경
BOOL kEndTask(QWORD qwTaskID);                        // 태스크 종료
void kExitTask(void);                                 // 태스크가 실행중인 자신을 종료
int kGetReadyTaskCount(void); // 실행 준비 태스크 수 반환
int kGetTaskCount(void);      // 총 태스크 수 반환(총 태스크 수 = 실행 준비 태스크 수 + 종료 대기 태스크 수 + 현재 실행중인 태스크 수)
TCB* kGetTCBInTCBPool(int iOffset);
BOOL kIsTaskExist(QWORD qwTaskID);
QWORD kGetProcessorLoad(void);
static TCB* kGetProcessByThread(TCB* pstThread); // 자신이 속한 프로세스 취득(프로세스인 경우 자신을 리턴하고, 스레드인 경우 부모 프로세스를 리턴함)

// 유휴 태스크 관련 함수
void kIdleTask(void);
void kHaltProcessorByLoad(void);

// FPU 관련 함수
QWORD kGetLastFPUUsedTaskID(void);
void kSetLastFPUUsedTaskID(QWORD qwTaskID);

#endif // __TASK_H__
