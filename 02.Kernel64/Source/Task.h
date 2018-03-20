#ifndef __TASK_H__
#define __TASK_H__

#include "Types.h"
#include "List.h"
#include "Synchronization.h"

/***** ¸ÅÅ©·Î Á¤ÀÇ *****/
// ÄÜÅØ½ºÆ® ·¹Áö½ºÅÍ °³¼ö¿Í Å©±â
#define TASK_REGISTERCOUNT 24 // 24 = 5(SS, RSP, RFLAGS, CS, RIP) + 19(ISR¿¡¼­ ÀúÀåÇÏ´Â ·¹Áö½ºÅÍµé)
#define TASK_REGISTERSIZE  8

// CONTEXT ÀÚ·á±¸Á¶ÀÇ ·¹Áö½ºÅÍ ¿ÀÇÁ¼Â
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

// ÅÂ½ºÅ© Ç® °ü·Ã ¸ÅÅ©·Î
#define TASK_TCBPOOLADDRESS 0x800000 // 8M
#define TASK_MAXCOUNT       1024

// ½ºÅÃ Ç® °ü·Ã ¸ÅÅ©·Î
#define TASK_STACKPOOLADDRESS (TASK_TCBPOOLADDRESS + (sizeof(TCB) * TASK_MAXCOUNT))
#define TASK_STACKSIZE        8192 // 8KB

// À¯È¿ÇÏÁö ¾ÊÀº ÅÂ½ºÅ© ID
#define TASK_INVALIDID     0xFFFFFFFFFFFFFFFF

// ÅÂ½ºÅ©°¡ ÇÑ¹ø¿¡ ÃÖ´ë·Î ¾µ ¼ö ÀÖ´Â ÇÁ·Î¼¼¼­ ½Ã°£(5ms)
#define TASK_PROCESSORTIME 5

// ÁØºñ ¸®½ºÆ® °³¼ö
#define TASK_MAXREADYLISTCOUNT 5

// ÅÂ½ºÅ© ¿ì¼±¼øÀ§ (ÅÂ½ºÅ© ÇÃ·¡±×(64ºñÆ®)ÀÇ ÇÏÀ§ 8ºñÆ® )
#define TASK_FLAGS_HIGHEST 0    // °¡Àå ³ôÀ½
#define TASK_FLAGS_HIGH    1    // ³ôÀ½
#define TASK_FLAGS_MEDIUM  2    // Áß°£
#define TASK_FLAGS_LOW     3    // ³·À½
#define TASK_FLAGS_LOWEST  4    // °¡Àå ³·À½
#define TASK_FLAGS_WAIT    0xFF // Á¾·á ÅÂ½ºÅ© ¿ì¼±¼øÀ§

// ÅÂ½ºÅ© ÇÃ·¡±×
#define TASK_FLAGS_ENDTASK 0x8000000000000000 // Á¾·á ÅÂ½ºÅ©
#define TASK_FLAGS_SYSTEM  0x4000000000000000 // ½Ã½ºÅÛ ÅÂ½ºÅ©
#define TASK_FLAGS_PROCESS 0x2000000000000000 // ÇÁ·Î¼¼½º
#define TASK_FLAGS_THREAD  0x1000000000000000 // ½º·¹µå
#define TASK_FLAGS_IDLE    0x0800000000000000 // À¯ÈÞ ÅÂ½ºÅ©

// ¸ÅÆ®·Î ÇÔ¼ö
#define GETPRIORITY(x)           ((x) & 0xFF)                                     // TCB.qwFlags(64ºñÆ®)ÀÇ ÇÏÀ§ 8ºñÆ® ÃßÃâ
#define SETPRIORITY(x, priority) ((x) = ((x) & 0xFFFFFFFFFFFFFF00) | (priority))  // TCB.qwFlags(64ºñÆ®)ÀÇ ÇÏÀ§ 8ºñÆ® ¼³Á¤
#define GETTCBOFFSET(x)          ((x) & 0xFFFFFFFF)                               // TCB.stLink.qwID(64ºñÆ®)ÀÇ ÇÏÀ§ 32ºñÆ® ÃßÃâ
#define GETTCBFROMTHREADLINK(x)  (TCB*)((QWORD)(x) - offsetof(TCB, stThreadLink)) // TCB.stThreadLink ¾îµå·¹½º¸¦ ÅëÇØ, TCB ¾îµå·¹½º¸¦ °è»êÇÏ¿© ¹ÝÈ¯ÇÔ


// 프로세서 친화도 필드에 아래의 값이 설정되면, 해당 태스크는 특별한 요구사항이 없는
// 것으로 판단하고 태스크 부하 분산 수행
#define TASK_LOADBALANCINGID	0xff



/***** ±¸Á¶Ã¼ Á¤ÀÇ *****/
#pragma pack(push, 1)

typedef struct kContextStruct{
	QWORD vqwRegister[TASK_REGISTERCOUNT];
} CONTEXT;

typedef struct kTaskControlBlockStruct{
	LISTLINK stLink;            // ½ºÄÉÁÙ·¯ ¸µÅ© -> ´ÙÀ½ ÅÂ½ºÅ© À§Ä¡(stLink.pvNext)
	                            //         -> ÅÂ½ºÅ© ID(stLink.qwID) -> »óÀ§ 32ºñÆ®: TCB ÇÒ´ç È½¼ö
	                            //                                -> ÇÏÀ§ 32ºñÆ®: TCB ¿ÀÇÁ¼Â
	                            //         [ÁÖÀÇ:LISTLINK°¡ ¹Ýµå½Ã ±¸Á¶Ã¼ÀÇ ¸Ç ¾Õ¿¡ À§Ä¡ÇØ¾ß ÇÔ]
	QWORD qwFlags;              // ÅÂ½ºÅ© ÇÃ·¡±× -> ºñÆ® 63: Á¾·á ÅÂ½ºÅ© ¿©ºÎ
                                //         -> ºñÆ® 62: ½Ã½ºÅÛ ÅÂ½ºÅ© ¿©ºÎ
	                            //         -> ºñÆ® 61: ÇÁ·Î¼¼¼­ ¿©ºÎ
	                            //         -> ºñÆ® 60: ½º·¹µå ¿©ºÎ
	                            //         -> ºñÆ® 59: À¯ÈÞ ÅÂ½ºÅ© ¿©ºÎ
	                            //         -> ºñÆ® 7~0: ÅÂ½ºÅ© ¿ì¼±¼øÀ§
	void* pvMemoryAddress;      // ÇÁ·Î¼¼½º ¸Þ¸ð¸® ¿µ¿ª ½ÃÀÛ ¾îµå·¹½º(ÄÚµå, µ¥ÀÌÅÍ ¿µ¿ª)
	QWORD qwMemorySize;         // ÇÁ·Î¼¼¼­ ¸Þ¸ð¸® ¿µ¿ª Å©±â(ÄÚµå, µ¥ÀÌÅÍ ¿µ¿ª)
	//--------------------------------------------------
	// ÀÌÇÏ´Â ½º·¹µå Á¤º¸
	//--------------------------------------------------
	LISTLINK stThreadLink;      // ÀÚ½Ä ½º·¹µå ¸µÅ©(´ÙÀ½ ÀÚ½Ä ½º·¹µå À§Ä¡, ½º·¹µå ID)
	QWORD qwParentProcessID;    // ºÎ¸ð ÇÁ·Î¼¼½º ID
	QWORD vqwFPUContext[512/8]; // FPU ÄÜÅØ½ºÆ® -> [ÁÖÀÇ]FPU ÄÜÅØ½ºÆ®ÀÇ ½ÃÀÛ ¾îµå·¹½º´Â ¹Ýµå½Ã 16ÀÇ ¹è¼ö¿©¾ßÇÔ. ±×·¯±â À§ÇØ¼­´Â ¾Æ·¡ 3°¡Áö Á¶°ÇÀ» ¸¸Á·½ÃÄÑ¾ß ÇÔ.
	                            // (512 byte °íÁ¤)   Á¶°Ç 1: TCB Ç®ÀÇ ½ÃÀÛ ¾îµå·¹½º°¡ 16ÀÇ ¹è¼ö¿©¾ßÇÔ (ÇöÀç 0x800000(8MB))
	                            //                  Á¶°Ç 2: °¢ TCBÀÇ Å©±â°¡ 16ÀÇ ¹è¼ö¿©¾ßÇÔ (ÇöÀç 816B)
                                //                  Á¶°Ç 3: °¢ TCB¿¡¼­ FPU ÄÜÅØ½ºÆ®ÀÇ ¿ÀÇÁ¼ÂÀÌ 16ÀÇ ¹è¼ö¿©¾ßÇÔ (ÇöÀç 64B)
	                            //                  ÇöÀç À§ 3°¡Áö Á¶°ÇÀ» ¸¸Á·ÇÏ¹Ç·Î, ÀÌÈÄ¿¡ TCB¿¡ ÇÊµå Ãß°¡½Ã ¹Ýµå½Ã vqwFPUContextÀÇ ¾Æ·¡¿¡ Ãß°¡ÇØ¾ß ÇÔ.
	LIST stChildThreadList;     // ÀÚ½Ä ½º·¹µå ¸®½ºÆ®
	CONTEXT stContext;          // ÄÜÅØ½ºÆ®
	void* pvStackAddress;       // ½ºÅÃ ½ÃÀÛ ¾îµå·¹½º
	QWORD qwStackSize;          // ½ºÅÃ Å©±â
	BOOL bFPUUsed;              // FPU »ç¿ë ¿©ºÎ (ÇØ´ç ÅÂ½ºÅ©¿¡¼­ ÀÌÀü¿¡ FPU ¿¬»êÀ» »ç¿ëÇÑ ÀûÀÌ ÀÖ´ÂÁö ¿©ºÎ)

	// 프로세서 친화도
	BYTE bAffinity;
	// 현재 태스크를 수행하는 로컬 APIC ID
	BYTE bAPICID;
	// TCB 전체를 16바이트로 맞추기 위한 패딩
	char vcPadding[9];
} TCB; // LISTITEM¿¡ ÇØ´ç, ÇöÀç TCB Å©±â´Â 816 byteÀÓ

typedef struct kTCBPoolManagerStruct{
	SPINLOCK stSpinLock;

	TCB* pstStartAddress; // TCB Ç® ½ÃÀÛ ÁÖ¼Ò
	int iMaxCount;        // TCB ÃÖ´ë °³¼ö(TCB Ç®¿¡¼­ ÃÖ´ë TCB °³¼ö)
	int iUseCount;        // TCB »ç¿ë °³¼ö(TCB Ç®¿¡¼­ »ç¿ëÁßÀÎ TCB °³¼ö)

	int iAllocatedCount;  // TCB ÇÒ´ç È½¼ö(TCB Ç®¿¡¼­ TCB°¡ ÇÒ´çµÈ È½¼ö)
} TCBPOOLMANAGER;

typedef struct kSchedulerStruct{
	SPINLOCK stSpinLock;

	TCB* pstRunningTask;                        // ÇöÀç ½ÇÇàÁßÀÎ ÅÂ½ºÅ©
	int iProcessorTime;                         // ÇöÀç ½ÇÇàÁßÀÎ ÅÂ½ºÅ©°¡ »ç¿ëÇÒ ¼ö ÀÖ´Â ÇÁ·Î¼¼¼­ ½Ã°£
	LIST vstReadyList[TASK_MAXREADYLISTCOUNT];  // ÁØºñ ¸®½ºÆ®(½ÇÇàÇÒ ÅÂ½ºÅ©°¡ ÁØºñÁßÀÎ ¸®½ºÆ®, ÅÂ½ºÅ© ¿ì¼±¼øÀ§¿¡ µû¶ó ±¸ºÐ)
	LIST stWaitList;                            // ´ë±â ¸®½ºÆ®(Á¾·áÇÒ ÅÂ½ºÅ©°¡ ´ë±âÁßÀÎ ¸®½ºÆ®)
	int viExecuteCount[TASK_MAXREADYLISTCOUNT]; // ¿ì¼±¼øÀ§º° ÅÂ½ºÅ© ½ÇÇà È½¼ö
	QWORD qwProcessorLoad;                      // ÇÁ·Î¼¼¼­ ºÎÇÏ(ÇÁ·Î¼¼¼­ »ç¿ë·ü)
	QWORD qwSpendProcessorTimeInIdleTask;       // À¯ÈÞ ÅÂ½ºÅ©°¡ »ç¿ëÇÑ ÇÁ·Î¼¼¼­ ½Ã°£
	QWORD qwLastFPUUsedTaskID;                  // ¸¶Áö¸· FPU »ç¿ë ÅÂ½ºÅ© ID
	BOOL bUseLoadBalancing;						// 부하 분산 사용 여부

} SCHEDULER;

#pragma pack(pop)

/***** ÇÔ¼ö Á¤ÀÇ *****/
// ÅÂ½ºÅ© °ü·Ã ÇÔ¼ö
static void kInitializeTCBPool(void);
static TCB* kAllocateTCB(void);
static void kFreeTCB(QWORD qwID);
TCB* kCreateTask(QWORD qwFlags, void* pvMemoryAddress, QWORD qwMemorySize, QWORD qwEntryPointAddress, BYTE bAffinity);
static void kSetUpTask(TCB* pstTCB, QWORD qwFlags, QWORD qwEntryPointAddress, void* pvStackAddress, QWORD qwStackSize);

// ½ºÄÉÁÙ·¯ °ü·Ã ÇÔ¼ö
void kInitializeScheduler(void);
void kSetRunningTask(BYTE,TCB* pstTask);
TCB* kGetRunningTask(BYTE);
static TCB* kGetNextTaskToRun(BYTE);           // ÁØºñ ¸®½ºÆ®¿¡¼­ ´ÙÀ½¿¡ ½ÇÇàÇÒ ÅÂ½ºÅ© ¼±ÅÃ(»èÁ¦)
static BOOL kAddTaskToReadyList(BYTE, TCB* pstTask); // ÁØºñ ¸®½ºÆ®¿¡ ÅÂ½ºÅ© Ãß°¡
BOOL kSchedule(void);                          // ÅÂ½ºÅ© ÀüÈ¯(ÅÂ½ºÅ© ¼öÇà½Ã)
BOOL kScheduleInInterrupt(void);               // ÅÂ½ºÅ© ÀüÈ¯(ÀÎÅÍ·´Æ® ÇÚµé·¯ ¼öÇà½Ã)
void kDecreaseProcessorTime(BYTE);
BOOL kIsProcessorTimeExpired(BYTE);
static TCB* kRemoveTaskFromReadyList(BYTE, QWORD qwTaskID); // ÁØºñ ¸®½ºÆ®¿¡¼­ ÅÂ½ºÅ© »èÁ¦
static BOOL kFindSchedulerOfTaskAndLock(QWORD, BYTE*);
BOOL kChangePriority(QWORD qwTaskID, BYTE bPriority); // ÅÂ½ºÅ© ¿ì¼±¼øÀ§ º¯°æ
BOOL kEndTask(QWORD qwTaskID);                        // ÅÂ½ºÅ© Á¾·á
void kExitTask(void);                                 // ÅÂ½ºÅ©°¡ ½ÇÇàÁßÀÎ ÀÚ½ÅÀ» Á¾·á
int kGetReadyTaskCount(BYTE); // ½ÇÇà ÁØºñ ÅÂ½ºÅ© ¼ö ¹ÝÈ¯
int kGetTaskCount(BYTE);      // ÃÑ ÅÂ½ºÅ© ¼ö ¹ÝÈ¯(ÃÑ ÅÂ½ºÅ© ¼ö = ½ÇÇà ÁØºñ ÅÂ½ºÅ© ¼ö + Á¾·á ´ë±â ÅÂ½ºÅ© ¼ö + ÇöÀç ½ÇÇàÁßÀÎ ÅÂ½ºÅ© ¼ö)
TCB* kGetTCBInTCBPool(int iOffset);
BOOL kIsTaskExist(QWORD qwTaskID);
QWORD kGetProcessorLoad(BYTE);
static TCB* kGetProcessByThread(TCB* pstThread); // ÀÚ½ÅÀÌ ¼ÓÇÑ ÇÁ·Î¼¼½º Ãëµæ(ÇÁ·Î¼¼½ºÀÎ °æ¿ì ÀÚ½ÅÀ» ¸®ÅÏÇÏ°í, ½º·¹µåÀÎ °æ¿ì ºÎ¸ð ÇÁ·Î¼¼½º¸¦ ¸®ÅÏÇÔ)
void kAddTaskToSchedulerWithLoadBalancing(TCB*);
static BYTE kFindSchedulerOfMinimumTaskCount(const TCB*);
BOOL kChangeProcessorAffinity(QWORD, BYTE);

// 유휴 태스크 관련
void kIdleTask(void);
void kHaltProcessorByLoad(BYTE);

// FPU 관련
QWORD kGetLastFPUUsedTaskID(BYTE);
void kSetLastFPUUsedTaskID(BYTE, QWORD qwTaskID);

#endif // __TASK_H__
