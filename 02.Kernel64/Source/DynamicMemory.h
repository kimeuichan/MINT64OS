#ifndef __DYNAMICMEMORY_H__
#define __DYNAMICMEMORY_H__

#include "Types.h"
#include "Task.h"

#define DYNAMICMEMORY_START_ADDRESS ((TASK_STACKPOOLADDRESS + (TASK_STACKSIZE * TASK_MAXCOUNT) + 0xfffff) & 0xfffffffffff00000)


#define DYNAMICMEMORY_MIN_SIZE				(1*1024)

#define DYNAMICMEMORY_EXIST					0x01
#define DYNAMICMEMORY_EMPTY					0x00


// 비트맵을 관리하는 자료구조
typedef struct kBitmapStruct{
	BYTE* pbBitmap;
	QWORD qwExistBitCount;
} BITMAP;


// 버디 블록을 관리하는 자료구조
typedef struct kDynamicMemoryManagerStruct{
	// 블록 리스트의 총 개수와 크기가 가장 작은 블록의 개수, 그리고 할당된 메모리 크기
	int iMaxLevelCount;
	int iBlockCountOfSmallestBlock;
	QWORD qwUsedSize;

	// 블록 풀의 시작 어드레스와 마지막 어드레스
	QWORD qwStartAddress;
	QWORD qwEndAddress;

	// 할당된 메모리가 속한 블록 리스트의 인덱스를 저장하는 영역과 비트맵 자료구조의 어드레스
	BYTE* pbAllocatedBlockListIndex;
	BITMAP* pstBitmapOfLevel;
} DYNAMICMEMORY;

void kInitializeDynamicMemory(void);
static QWORD kCalculateDynamicMemorySize(void);
static int kCalculateMetaBlockCount(QWORD qwDynamicRAMSize);
void* kAllocateMemory(QWORD qwSize);
static QWORD kGetBuddyBlockSize(QWORD qwSize);
static int kAllocateBuddyBlock(QWORD qwAlignedSize);
static int kGetBlockListIndexOfMatchSize(QWORD qwAlignedSize);
static int kFindFreeBlockInBitmap(int iBlockListIndex);
static void kSetFlagInBitmap(int iBlockListIndex, int iOffset, BYTE bFlag);
BOOL kFreeMemory(void* pvAddress);
static BOOL kFreeBuddyBlock(int iBlockListIndex, int iBlockOffset);
static BYTE kGetFlagInBitmap(int iBlockListIndex, int iOffset);
void kGetDynamicMemoryInformation(QWORD* pqwDynamicMemoryStartAddress, QWORD* pqwDynamicMemoryTotalSize, QWORD* pqwMetaDataSize, QWORD* pqwUsedMemorySize);
DYNAMICMEMORY* kGetDynamicMemoryManager(void);
#endif