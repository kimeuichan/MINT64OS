#ifndef __DYNAMIC_MEMORY_H__
#define __DYNAMIC_MEMORY_H__

#include "Types.h"
#include "Task.h"
#include "Synchronization.h"

/***** 매크로 정의 *****/
// 동적 메모리 영역 시작 어드레스(0x1100000, 17MB): 1MB 단위로 정렬(1MB의 배수로 설정, 올림 처리), TCB 자료구조의 크기가 1KB이하라는 조건하에서 17MB가 됨
#define DYNAMICMEMORY_START_ADDRESS ((TASK_STACKPOOLADDRESS + (TASK_STACKSIZE * TASK_MAXCOUNT) + 0xfffff) & 0xfffffffffff00000)

// 최소 블록 크기(1KB)
#define DYNAMICMEMORY_MIN_SIZE (1 * 1024)

// 비트맵 플래그
#define DYNAMICMEMORY_EXIST 0x01 // 블록 존재(블록이 할당 가능한 상태)
#define DYNAMICMEMORY_EMPTY 0x00 // 블록 부재(블록이 할당 되었거나 결합되어 할당 불가능한 상태)

/***** 구조체 정의 *****/
#pragma pack(push, 1)

typedef struct kBitmapStruct{
	BYTE* pbBitmap;        // 실제 비트맵의 어드레스
	QWORD qwExistBitCount; // 존재하는 비트수(해당 비트맵에서 1인 비트의 개수)
} BITMAP;

typedef struct kDynamicMemoryManagerStruct{
	int iMaxLevelCount;              // 블록 리스트 개수(레벨 개수)
	int iBlockCountOfSmallestBlock;  // 최소 블록 개수
	QWORD qwUsedSize;                // 할당된 메모리 크기(사용된 메모리 크기)
	QWORD qwStartAddress;            // 블록 풀 시작 어드레스
	QWORD qwEndAddredss;             // 블록 풀 끝 어드레스
	BYTE* pbAllocatedBlockListIndex; // 인덱스 영역의 어드레스(할당된 블록의 블록 리스트 인덱스를 저장하는 영역의 어드레스)
	BITMAP* pstBitmapOfLevel;        // 비트맵 자료구조의 어드레스

	SPINLOCK stSpinLock;
} DYNAMICMEMORY;

#pragma pack(pop)

/***** 함수 정의 *****/
void kInitializeDynamicMemory(void);
void* kAllocateMemory(QWORD qwSize);
BOOL kFreeMemory(void* pvAddress);
void kGetDynamicMemoryInformation(QWORD* pqwDynamicMemoryStartAddress, QWORD* pqwDynamicMemoryTotalSize, QWORD* pqwMetaDataSize, QWORD* pqwUsedMemorySize);
DYNAMICMEMORY* kGetDynamicMemoryManager(void);
static QWORD kCalculateDynamicMemorySize(void);
static int kCalculateMetaBlockCount(QWORD qwDynamicRAMSize);
static int kAllocateBuddyBlock(QWORD qwAlignedSize);
static QWORD kGetBuddyBlockSize(QWORD qwSize);
static int kGetBlockListIndexOfMatchSize(QWORD qwAlignedSize);
static int kFindFreeBlockInBitmap(int iBlockListIndex);
static void kSetFlagInBitmap(int iBlockListIndex, int iOffset, BYTE bFlag);
static BOOL kFreeBuddyBlock(int iBlockListIndex, int iBlockOffset);
static BYTE kGetFlagInBitmap(int iBlockListIndex, int iOffset);

#endif // __DYNAMIC_MEMORY_H__
