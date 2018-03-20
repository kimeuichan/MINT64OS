#ifndef __DYNAMIC_MEMORY_H__
#define __DYNAMIC_MEMORY_H__

#include "Types.h"
#include "Task.h"
#include "Synchronization.h"

/***** ��ũ�� ���� *****/
// ���� �޸� ���� ���� ��巹��(0x1100000, 17MB): 1MB ������ ����(1MB�� ����� ����, �ø� ó��), TCB �ڷᱸ���� ũ�Ⱑ 1KB���϶�� �����Ͽ��� 17MB�� ��
#define DYNAMICMEMORY_START_ADDRESS ((TASK_STACKPOOLADDRESS + (TASK_STACKSIZE * TASK_MAXCOUNT) + 0xfffff) & 0xfffffffffff00000)

// �ּ� ��� ũ��(1KB)
#define DYNAMICMEMORY_MIN_SIZE (1 * 1024)

// ��Ʈ�� �÷���
#define DYNAMICMEMORY_EXIST 0x01 // ��� ����(����� �Ҵ� ������ ����)
#define DYNAMICMEMORY_EMPTY 0x00 // ��� ����(����� �Ҵ� �Ǿ��ų� ���յǾ� �Ҵ� �Ұ����� ����)

/***** ����ü ���� *****/
#pragma pack(push, 1)

typedef struct kBitmapStruct{
	BYTE* pbBitmap;        // ���� ��Ʈ���� ��巹��
	QWORD qwExistBitCount; // �����ϴ� ��Ʈ��(�ش� ��Ʈ�ʿ��� 1�� ��Ʈ�� ����)
} BITMAP;

typedef struct kDynamicMemoryManagerStruct{
	int iMaxLevelCount;              // ��� ����Ʈ ����(���� ����)
	int iBlockCountOfSmallestBlock;  // �ּ� ��� ����
	QWORD qwUsedSize;                // �Ҵ�� �޸� ũ��(���� �޸� ũ��)
	QWORD qwStartAddress;            // ��� Ǯ ���� ��巹��
	QWORD qwEndAddredss;             // ��� Ǯ �� ��巹��
	BYTE* pbAllocatedBlockListIndex; // �ε��� ������ ��巹��(�Ҵ�� ����� ��� ����Ʈ �ε����� �����ϴ� ������ ��巹��)
	BITMAP* pstBitmapOfLevel;        // ��Ʈ�� �ڷᱸ���� ��巹��

	SPINLOCK stSpinLock;
} DYNAMICMEMORY;

#pragma pack(pop)

/***** �Լ� ���� *****/
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
