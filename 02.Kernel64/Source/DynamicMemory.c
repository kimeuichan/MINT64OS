#include "DynamicMemory.h"

/***** ���� ���� ���� *****/
static DYNAMICMEMORY gs_stDynamicMemory;

void kInitializeDynamicMemory(void){
	QWORD qwDynamicMemorySize;
	int i, j;
	BYTE* pbCurrentBitmapPosition;
	int iBlockCountOfLevel, iMetaBlockCount;

	qwDynamicMemorySize = kCalculateDynamicMemorySize();
	iMetaBlockCount = kCalculateMetaBlockCount(qwDynamicMemorySize);

	// �ּ� ��� ���� ����
	gs_stDynamicMemory.iBlockCountOfSmallestBlock = (qwDynamicMemorySize / DYNAMICMEMORY_MIN_SIZE) - iMetaBlockCount;

	// �ּ� ��� ������ 2^i���� ���������� ������ �����ν�, ��� ����Ʈ ������ ����
	for(i = 0; (gs_stDynamicMemory.iBlockCountOfSmallestBlock >> i) > 0; i++){
		;
	}
	gs_stDynamicMemory.iMaxLevelCount = i;

	//----------------------------------------------------------------------------------------------------
	// �ε��� ���� �ʱ�ȭ
	//----------------------------------------------------------------------------------------------------
	gs_stDynamicMemory.pbAllocatedBlockListIndex = (BYTE*)DYNAMICMEMORY_START_ADDRESS;
	for(i = 0; i < gs_stDynamicMemory.iBlockCountOfSmallestBlock; i++){
		gs_stDynamicMemory.pbAllocatedBlockListIndex[i] = 0xFF;
	}

	// ��Ʈ�� �ڷᱸ���� ��巹�� ����
	gs_stDynamicMemory.pstBitmapOfLevel = (BITMAP*)(DYNAMICMEMORY_START_ADDRESS + (sizeof(BYTE) * gs_stDynamicMemory.iBlockCountOfSmallestBlock));

	// ���� ��Ʈ���� ��巹�� ����
	pbCurrentBitmapPosition = ((BYTE*)gs_stDynamicMemory.pstBitmapOfLevel) + (sizeof(BITMAP) * gs_stDynamicMemory.iMaxLevelCount);

	// ��� ����Ʈ���� ������ ���鼭 ��Ʈ���� �ʱ�ȭ (�ʱ� ���´� ���� ū ��ϰ� ������ ��ϸ� �����ϹǷ� �������� ���� ���·� ����)
	for(j = 0; j < gs_stDynamicMemory.iMaxLevelCount; j++){

		//----------------------------------------------------------------------------------------------------
		// ��Ʈ�� �ڷᱸ�� �ʱ�ȭ
		//----------------------------------------------------------------------------------------------------
		gs_stDynamicMemory.pstBitmapOfLevel[j].pbBitmap = pbCurrentBitmapPosition;
		gs_stDynamicMemory.pstBitmapOfLevel[j].qwExistBitCount = 0;

		//----------------------------------------------------------------------------------------------------
		// ���� ��Ʈ�� �ʱ�ȭ
		//----------------------------------------------------------------------------------------------------
		iBlockCountOfLevel = gs_stDynamicMemory.iBlockCountOfSmallestBlock >> j;

		// ��� ������ 8�� �̻��� ���, ���� ������� ���յ� �� �����Ƿ� ���� ���·� ����
		for(i = 0; i < (iBlockCountOfLevel / 8); i++){
			*pbCurrentBitmapPosition = DYNAMICMEMORY_EMPTY;
			pbCurrentBitmapPosition++;
		}

		// ��� ������ 8�� ������ �������� �ʴ� ������ ��ϵ鿡 ���� ó��
		if((iBlockCountOfLevel % 8) != 0){
			*pbCurrentBitmapPosition = DYNAMICMEMORY_EMPTY;

			// ������ ��� ������ Ȧ������� ������ �� ����� ���� ������� ���յ��� �� ��. ����, ������ �� ����� ���� ��� ����Ʈ�� �����ϴ� ������ ������� ����
			i = iBlockCountOfLevel % 8;
			if((i % 2) == 1){
				*pbCurrentBitmapPosition |= (DYNAMICMEMORY_EXIST << (i - 1));
				gs_stDynamicMemory.pstBitmapOfLevel[j].qwExistBitCount = 1;
			}

			pbCurrentBitmapPosition++;
		}
	}

	// ��� Ǯ  ��巹���� �Ҵ�� �޸� ũ�� ����
	gs_stDynamicMemory.qwStartAddress = DYNAMICMEMORY_START_ADDRESS + (iMetaBlockCount * DYNAMICMEMORY_MIN_SIZE);
	gs_stDynamicMemory.qwEndAddredss = DYNAMICMEMORY_START_ADDRESS + kCalculateDynamicMemorySize();
	gs_stDynamicMemory.qwUsedSize = 0;
}

static QWORD kCalculateDynamicMemorySize(void){
	QWORD qwRAMSize;

	qwRAMSize = (kGetTotalRAMSize() * 1024 * 1024);

	// 3GB�̻��� �޸𸮿��� �׷��� ���� �޸𸮿� ���μ����� ����ϴ� �������Ͱ� �����ϹǷ�, �ִ� 3GB������ ���� �޸� �������� ���
	if(qwRAMSize > ((QWORD)3 * 1024 * 1024 * 1024)){
		qwRAMSize = ((QWORD)3 * 1024 * 1024 * 1024);
	}

	return qwRAMSize - DYNAMICMEMORY_START_ADDRESS;
}

static int kCalculateMetaBlockCount(QWORD qwDynamicRAMSize){
	long lBlockCountOfSmallestBlock;
	DWORD dwSizeOfAllocatedBlockListIndex;
	DWORD dwSizeOfBitmap;
	long i;

	lBlockCountOfSmallestBlock = qwDynamicRAMSize / DYNAMICMEMORY_MIN_SIZE;

	// �ε��� ������ ũ�� ���
	dwSizeOfAllocatedBlockListIndex = lBlockCountOfSmallestBlock * sizeof(BYTE);

	dwSizeOfBitmap = 0;
	for(i = 0; (lBlockCountOfSmallestBlock >> i) > 0; i++){
		// ��Ʈ�� �ڷᱸ�� ������ ũ�� ���
		dwSizeOfBitmap += sizeof(BITMAP);

		// ���� ��Ʈ�� ������ ũ�� ���(����Ʈ ������ ����, �ø� ó��)
		dwSizeOfBitmap += ((lBlockCountOfSmallestBlock >> i) + 7) / 8;
	}

	// ��Ÿ ��� ������ ũ�⸦ �ּ� ��� ���� ������ ����(�ø� ó��)
	return (dwSizeOfAllocatedBlockListIndex + dwSizeOfBitmap + (DYNAMICMEMORY_MIN_SIZE - 1)) / DYNAMICMEMORY_MIN_SIZE;
}

void* kAllocateMemory(QWORD qwSize){
	QWORD qwAlignedSize;     // ���� ��� ũ�� ������ ���ĵ� ũ��
	QWORD qwRelativeAddress; // ��� Ǯ ���� ��巹���� �������� �� ��� �ּ�
	long lOffset;            // �Ҵ�� ����� ��Ʈ�� ������
	int iSizeArrayOffset;    // �Ҵ�� ����� ����Ʈ ���� ������
	int iIndexOfBlockList;   // ��� ũ�Ⱑ ��ġ�ϴ� ��� ����Ʈ �ε���(����)

	// �Ҵ���� �޸� ũ�⿡ ���� ����� ���� ��� ũ�⸦ �˻�
	qwAlignedSize = kGetBuddyBlockSize(qwSize);
	if(qwAlignedSize == 0){
		return NULL;
	}

	// ���� �޸𸮰� ������� �ʴ� ���, ����
	if((gs_stDynamicMemory.qwStartAddress + gs_stDynamicMemory.qwUsedSize + qwAlignedSize) > gs_stDynamicMemory.qwEndAddredss){
		return NULL;
	}

	// ���� ����� �Ҵ��ϰ�, �Ҵ�� ����� ���� ��� ����Ʈ�� ��Ʈ�� �����¸� ��ȯ
	lOffset = kAllocateBuddyBlock(qwAlignedSize);
	if(lOffset == -1){
		return NULL;
	}

	// ��� ũ�Ⱑ ��ġ�ϴ� ��� ����Ʈ�� �ε����� �˻�
	iIndexOfBlockList = kGetBlockListIndexOfMatchSize(qwAlignedSize);

	// �ε��� ������ �Ҵ�� ����� ����Ʈ ���� ������ ��ġ�� ��� ����Ʈ �ε����� ���� (�޸𸮸� ������ ��, ��� ����Ʈ �ε����� ����Ͽ� �Ҵ�� �޸��� ũ�⸦ ����)
	qwRelativeAddress = qwAlignedSize * lOffset;
	iSizeArrayOffset = qwRelativeAddress / DYNAMICMEMORY_MIN_SIZE;
	gs_stDynamicMemory.pbAllocatedBlockListIndex[iSizeArrayOffset] = (BYTE)iIndexOfBlockList;

	// �Ҵ�� �޸� ũ�⸦ ����
	gs_stDynamicMemory.qwUsedSize += qwAlignedSize;

	// �Ҵ�� �޸��� ���� �ּ� ��ȯ(���� �ּ�=��� Ǯ ���� ��巹��+��� �ּ�)
	return (void*)(gs_stDynamicMemory.qwStartAddress + qwRelativeAddress);
}

static QWORD kGetBuddyBlockSize(QWORD qwSize){
	long i;

	// �Ķ���� �޸� ũ�⿡ ���� ����� ���� ��� ũ�⸦ �˻�
	for(i = 0; i < gs_stDynamicMemory.iMaxLevelCount; i++){
		if(qwSize <= (DYNAMICMEMORY_MIN_SIZE << i)){
			return (DYNAMICMEMORY_MIN_SIZE << i);
		}
	}

	return 0;
}

static int kAllocateBuddyBlock(QWORD qwAlignedSize){
	int iBlockListIndex; // ��� ũ�Ⱑ ��ġ�ϴ� ��� ����Ʈ �ε���(����)
	int iFreeOffset;     // �����ϴ� ����� ��Ʈ�� ������
	int i;
	BOOL bPreviousInterruptFlag;

	// ��� ũ�Ⱑ ��ġ�ϴ� ��� ����Ʈ�� �ε����� �˻�
	iBlockListIndex = kGetBlockListIndexOfMatchSize(qwAlignedSize);
	if(iBlockListIndex == -1){
		return -1;
	}

	bPreviousInterruptFlag = kLockForSystemData();

	// ��ġ�ϴ� ��� ����Ʈ���� �ֻ��� ��� ����Ʈ���� ������ �ö󰡸鼭, ���� ������ ����� ã��
	for(i = iBlockListIndex; i < gs_stDynamicMemory.iMaxLevelCount; i++){

		// ��� ����Ʈ�� ��Ʈ���� Ȯ���Ͽ�, ���� ������ ����� ã��
		iFreeOffset = kFindFreeBlockInBitmap(i);
		if(iFreeOffset != -1){
			break;
		}
	}

	// �ֻ��� ��� ����Ʈ���� �˻��ߴµ���, ���� ������ ����� ������ ����
	if(iFreeOffset == -1){
		kUnlockForSystemData(bPreviousInterruptFlag);
		return -1;
	}

	// ã�� ����� ���� ���·� ����
	kSetFlagInBitmap(i, iFreeOffset, DYNAMICMEMORY_EMPTY);

	// ���� ��� ����Ʈ���� ����� ã�� ���, ���� ����� ����
	if(i > iBlockListIndex){

		// ã�� ��� ����Ʈ���� ��ġ�ϴ� ��� ����Ʈ���� ������ �������鼭, ����� ����
		for(i = i - 1; i >= iBlockListIndex; i--){
			// ���� ����� ���� ���·� ����
			kSetFlagInBitmap(i, iFreeOffset * 2, DYNAMICMEMORY_EMPTY);

			// ������ ����� ���� ���·� ����
			kSetFlagInBitmap(i, iFreeOffset * 2 + 1, DYNAMICMEMORY_EXIST);

			// ���� ����� �ٽ� ����
			iFreeOffset = iFreeOffset * 2;
		}
	}

	kUnlockForSystemData(bPreviousInterruptFlag);

	return iFreeOffset;
}

static int kGetBlockListIndexOfMatchSize(QWORD qwAlignedSize){
	int i;

	// ��� ũ�Ⱑ ��ġ�ϴ� ��� ����Ʈ�� �ε����� �˻�(��� ����Ʈ �ε���=����)
	for(i = 0; i < gs_stDynamicMemory.iMaxLevelCount; i++){
		if(qwAlignedSize <= (DYNAMICMEMORY_MIN_SIZE << i)){
			return i;
		}
	}

	return -1;
}

static int kFindFreeBlockInBitmap(int iBlockListIndex){
	int i, iMaxCount;
	BYTE* pbBitmap;
	QWORD* pqwBitmap;

	// ��Ʈ�ʿ� �����ϴ� ��Ʈ��(1�� ��Ʈ��)�� 0���� ���, ����
	if(gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].qwExistBitCount == 0){
		return -1;
	}

	// ��� ����Ʈ�� ��� ������ ���� ��, �� ������ŭ ��Ʈ���� �˻�
	iMaxCount = gs_stDynamicMemory.iBlockCountOfSmallestBlock >> iBlockListIndex;
	pbBitmap = gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].pbBitmap;

	for(i = 0; i < iMaxCount;){

		// ��� ������ 64�� �̻��� ���, 64��Ʈ(QWORD)�� �Ѳ����� �˻��ؼ� 1�� ��Ʈ�� �ִ��� Ȯ��
		if(((iMaxCount - i) / 64) > 0){

			pqwBitmap = (QWORD*)&(pbBitmap[i/8]);

			// 64��Ʈ�� ��� 0�� ���, ��� ����
			if(*pqwBitmap == 0){
				i += 64;
				continue;
			}
		}

		// ��Ʈ���� ������ ��ġ�� ��Ʈ�� 1�� ���, �ش� �������� ��ȯ(�����ϴ� ����� ��Ʈ�� �������� ��ȯ)
		if((pbBitmap[i/8] & (DYNAMICMEMORY_EXIST << (i % 8))) != 0){
			return i;
		}

		i++;
	}

	return -1;
}

static void kSetFlagInBitmap(int iBlockListIndex, int iOffset, BYTE bFlag){
	BYTE* pbBitmap;

	pbBitmap = gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].pbBitmap;

	// ��� ���� ����
	if(bFlag == DYNAMICMEMORY_EXIST){
		// ��Ʈ���� ������ ��ġ�� ��Ʈ�� 0->1(��� ����->��� ����)�� ����Ǵ� ���, �����ϴ� ��Ʈ���� ����
		if((pbBitmap[iOffset/8] & (0x01 << (iOffset % 8))) == 0){
			gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].qwExistBitCount++;
		}

		// ��Ʈ���� ������ ��ġ�� ��Ʈ�� 1(��� ����)�� ����
		pbBitmap[iOffset/8] |= (0x01 << (iOffset % 8));

	// ��� ���� ����
	}else{
		// ��Ʈ���� ������ ��ġ�� ��Ʈ�� 1->0(��� ����->��� ����)���� ����Ǵ� ���, �����ϴ� ��Ʈ���� ����
		if((pbBitmap[iOffset/8] & (0x01 << (iOffset % 8))) != 0){
			gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].qwExistBitCount--;
		}

		// ��Ʈ���� ������ ��ġ�� ��Ʈ�� 0(��� ����)���� ����
		pbBitmap[iOffset/8] &= ~(0x01 << (iOffset % 8));
	}
}

BOOL kFreeMemory(void* pvAddress){
	QWORD qwRelativeAddress; // ������ ����� ��� �ּ�(��� Ǯ ���� ��巹���� �������� �� ��� �ּ�)
	int iSizeArrayOffset;    // ������ ����� ����Ʈ ���� ������
	QWORD qwBlockSize;       // ������ ����� ũ��
	int iBlockListIndex;     // ������ ����� ��� ����Ʈ �ε���
	int iBitmapOffset;       // ������ ����� ��Ʈ�� ������

	if(pvAddress == NULL){
		return FALSE;
	}

	// ������ ����� ��� �ּ�, ������ ����� ����Ʈ ���� �������� ���
	qwRelativeAddress = ((QWORD)pvAddress) - gs_stDynamicMemory.qwStartAddress;
	iSizeArrayOffset = qwRelativeAddress / DYNAMICMEMORY_MIN_SIZE;

	// �Ҵ�� ���°� �ƴ� ���, ����
	if(gs_stDynamicMemory.pbAllocatedBlockListIndex[iSizeArrayOffset] == 0xFF){
		return FALSE;
	}

	// ������ ����� ��� ����Ʈ �ε����� ������ ��, �ʱ�ȭ
	iBlockListIndex = (int)gs_stDynamicMemory.pbAllocatedBlockListIndex[iSizeArrayOffset];
	gs_stDynamicMemory.pbAllocatedBlockListIndex[iSizeArrayOffset] = 0xFF;

	// ������ ����� ũ�⸦ ���
	qwBlockSize = DYNAMICMEMORY_MIN_SIZE << iBlockListIndex;

	// ������ ����� ��� ����Ʈ �ε����� ��Ʈ�� �������� �̿��ؼ�, ���� ��� ����
	iBitmapOffset = qwRelativeAddress / qwBlockSize;
	if(kFreeBuddyBlock(iBlockListIndex, iBitmapOffset) == TRUE){

		// �Ҵ�� �޸� ũ�⸦ ����
		gs_stDynamicMemory.qwUsedSize -= qwBlockSize;
		return TRUE;
	}

	return FALSE;
}

// PARAM1 : iBlockListIndex      -> ������ ����� ��� ����Ʈ �ε���
// PARAM2 : iBlockOffset         -> ������ ����� ��� ������(��Ʈ�� ������)
static BOOL kFreeBuddyBlock(int iBlockListIndex, int iBlockOffset){
	int iBuddyBlockOffset;       // ���� ��� ������
	int i;                       // �ε���
	BYTE bFlag;                  // ���� ��� ���� �÷���
	BOOL bPreviousInterruptFlag; // ���� ���ͷ�Ʈ �÷���

	bPreviousInterruptFlag = kLockForSystemData();

	// ������ ��� ����Ʈ���� �ֻ��� ��� ����Ʈ���� ������ �ö󰡸鼭, ����� ����
	for(i = iBlockListIndex; i < gs_stDynamicMemory.iMaxLevelCount; i++){

		// ���� ����� ���� ���·� ����
		kSetFlagInBitmap(i, iBlockOffset, DYNAMICMEMORY_EXIST);

		// ��� �������� ¦��(����)�� Ȧ��(������)�� �˻��ϰ�, Ȧ��(������)�� ¦��(����)�� �˻��Ͽ�, ������ ����� �����Ѵٸ� ����
		if((iBlockOffset % 2) == 0){
			iBuddyBlockOffset = iBlockOffset + 1;

		}else{
			iBuddyBlockOffset = iBlockOffset - 1;
		}

		// ���� ����� ���� �÷��� ���
		bFlag = kGetFlagInBitmap(i, iBuddyBlockOffset);

		// ���� ����� ���� ������ ���, ����
		if(bFlag == DYNAMICMEMORY_EMPTY){
			break;
		}

		// ������ ����� ���� ������ ���, ����� ����(���� ��ϰ� ���� ����� ��� ���� ���·� �����ϰ�, ���� ��� ����Ʈ�� �̵�)
		kSetFlagInBitmap(i, iBuddyBlockOffset, DYNAMICMEMORY_EMPTY);
		kSetFlagInBitmap(i, iBlockOffset, DYNAMICMEMORY_EMPTY);

		// ���� ��� ����Ʈ�� ��� �������� �����ϰ�, ���� ������ ���� ��� ����Ʈ���� �ٽ� �ݺ�
		iBlockOffset = iBlockOffset / 2;
	}

	kUnlockForSystemData(bPreviousInterruptFlag);
	return TRUE;
}

static BYTE kGetFlagInBitmap(int iBlockListIndex, int iOffset){
	BYTE* pbBitmap;

	pbBitmap = gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].pbBitmap;

	// ��Ʈ���� ������ ��ġ�� ��Ʈ�� 1�� ���, ��� ���� ���� ��ȯ
	if((pbBitmap[iOffset/8] & (0x01 << (iOffset % 8))) != 0){
		return DYNAMICMEMORY_EXIST;
	}

	// ��Ʈ���� ������ ��ġ�� ��Ʈ�� 0�� ���, ��� ���� ���� ��ȯ
	return DYNAMICMEMORY_EMPTY;
}

void kGetDynamicMemoryInformation(QWORD* pqwDynamicMemoryStartAddress, QWORD* pqwDynamicMemoryTotalSize, QWORD* pqwMetaDataSize, QWORD* pqwUsedMemorySize){
	*pqwDynamicMemoryStartAddress = DYNAMICMEMORY_START_ADDRESS;
	*pqwDynamicMemoryTotalSize = kCalculateDynamicMemorySize();
	*pqwMetaDataSize = kCalculateMetaBlockCount(*pqwDynamicMemoryTotalSize) * DYNAMICMEMORY_MIN_SIZE;
	*pqwUsedMemorySize = gs_stDynamicMemory.qwUsedSize;
}

DYNAMICMEMORY* kGetDynamicMemoryManager(void){
	return &gs_stDynamicMemory;
}

