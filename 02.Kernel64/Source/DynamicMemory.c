#include "DynamicMemory.h"

/***** 전역 변수 정의 *****/
static DYNAMICMEMORY gs_stDynamicMemory;

void kInitializeDynamicMemory(void){
	QWORD qwDynamicMemorySize;
	int i, j;
	BYTE* pbCurrentBitmapPosition;
	int iBlockCountOfLevel, iMetaBlockCount;

	qwDynamicMemorySize = kCalculateDynamicMemorySize();
	iMetaBlockCount = kCalculateMetaBlockCount(qwDynamicMemorySize);

	// 최소 블록 개수 설정
	gs_stDynamicMemory.iBlockCountOfSmallestBlock = (qwDynamicMemorySize / DYNAMICMEMORY_MIN_SIZE) - iMetaBlockCount;

	// 최소 블록 개수를 2^i으로 점진적으로 나누어 감으로써, 블록 리스트 개수를 구함
	for(i = 0; (gs_stDynamicMemory.iBlockCountOfSmallestBlock >> i) > 0; i++){
		;
	}
	gs_stDynamicMemory.iMaxLevelCount = i;

	//----------------------------------------------------------------------------------------------------
	// 인덱스 영역 초기화
	//----------------------------------------------------------------------------------------------------
	gs_stDynamicMemory.pbAllocatedBlockListIndex = (BYTE*)DYNAMICMEMORY_START_ADDRESS;
	for(i = 0; i < gs_stDynamicMemory.iBlockCountOfSmallestBlock; i++){
		gs_stDynamicMemory.pbAllocatedBlockListIndex[i] = 0xFF;
	}

	// 비트맵 자료구조의 어드레스 설정
	gs_stDynamicMemory.pstBitmapOfLevel = (BITMAP*)(DYNAMICMEMORY_START_ADDRESS + (sizeof(BYTE) * gs_stDynamicMemory.iBlockCountOfSmallestBlock));

	// 실제 비트맵의 어드레스 설정
	pbCurrentBitmapPosition = ((BYTE*)gs_stDynamicMemory.pstBitmapOfLevel) + (sizeof(BITMAP) * gs_stDynamicMemory.iMaxLevelCount);

	// 블록 리스트별로 루프를 돌면서 비트맵을 초기화 (초기 상태는 가장 큰 블록과 자투리 블록만 존재하므로 나머지는 부재 상태로 설정)
	for(j = 0; j < gs_stDynamicMemory.iMaxLevelCount; j++){

		//----------------------------------------------------------------------------------------------------
		// 비트맵 자료구조 초기화
		//----------------------------------------------------------------------------------------------------
		gs_stDynamicMemory.pstBitmapOfLevel[j].pbBitmap = pbCurrentBitmapPosition;
		gs_stDynamicMemory.pstBitmapOfLevel[j].qwExistBitCount = 0;

		//----------------------------------------------------------------------------------------------------
		// 실제 비트맵 초기화
		//----------------------------------------------------------------------------------------------------
		iBlockCountOfLevel = gs_stDynamicMemory.iBlockCountOfSmallestBlock >> j;

		// 블록 개수가 8개 이상인 경우, 상위 블록으로 결합될 수 있으므로 부재 상태로 설정
		for(i = 0; i < (iBlockCountOfLevel / 8); i++){
			*pbCurrentBitmapPosition = DYNAMICMEMORY_EMPTY;
			pbCurrentBitmapPosition++;
		}

		// 블록 개수가 8로 나누어 떨어지지 않는 나머지 블록들에 대한 처리
		if((iBlockCountOfLevel % 8) != 0){
			*pbCurrentBitmapPosition = DYNAMICMEMORY_EMPTY;

			// 나머지 블록 개수가 홀수개라면 마지막 한 블록은 상위 블록으로 결합되지 못 함. 따라서, 마지막 한 블록을 현재 블록 리스트에 존재하는 자투리 블록으로 설정
			i = iBlockCountOfLevel % 8;
			if((i % 2) == 1){
				*pbCurrentBitmapPosition |= (DYNAMICMEMORY_EXIST << (i - 1));
				gs_stDynamicMemory.pstBitmapOfLevel[j].qwExistBitCount = 1;
			}

			pbCurrentBitmapPosition++;
		}
	}

	// 블록 풀  어드레스와 할당된 메모리 크기 설정
	gs_stDynamicMemory.qwStartAddress = DYNAMICMEMORY_START_ADDRESS + (iMetaBlockCount * DYNAMICMEMORY_MIN_SIZE);
	gs_stDynamicMemory.qwEndAddredss = DYNAMICMEMORY_START_ADDRESS + kCalculateDynamicMemorySize();
	gs_stDynamicMemory.qwUsedSize = 0;
}

static QWORD kCalculateDynamicMemorySize(void){
	QWORD qwRAMSize;

	qwRAMSize = (kGetTotalRAMSize() * 1024 * 1024);

	// 3GB이상의 메모리에는 그래픽 비디오 메모리와 프로세서가 사용하는 레지스터가 존재하므로, 최대 3GB까지만 동적 메모리 영역으로 사용
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

	// 인덱스 영역의 크기 계산
	dwSizeOfAllocatedBlockListIndex = lBlockCountOfSmallestBlock * sizeof(BYTE);

	dwSizeOfBitmap = 0;
	for(i = 0; (lBlockCountOfSmallestBlock >> i) > 0; i++){
		// 비트맵 자료구조 영역의 크기 계산
		dwSizeOfBitmap += sizeof(BITMAP);

		// 실제 비트맵 영역의 크기 계산(바이트 단위로 정렬, 올림 처리)
		dwSizeOfBitmap += ((lBlockCountOfSmallestBlock >> i) + 7) / 8;
	}

	// 메타 블록 영역의 크기를 최소 블록 개수 단위로 정렬(올림 처리)
	return (dwSizeOfAllocatedBlockListIndex + dwSizeOfBitmap + (DYNAMICMEMORY_MIN_SIZE - 1)) / DYNAMICMEMORY_MIN_SIZE;
}

void* kAllocateMemory(QWORD qwSize){
	QWORD qwAlignedSize;     // 버디 블록 크기 단위로 정렬된 크기
	QWORD qwRelativeAddress; // 블록 풀 시작 어드레스를 기준으로 한 상대 주소
	long lOffset;            // 할당된 블록의 비트맵 오프셋
	int iSizeArrayOffset;    // 할당된 블록의 바이트 단위 오프셋
	int iIndexOfBlockList;   // 블록 크기가 일치하는 블록 리스트 인덱스(레벨)

	// 할당받을 메모리 크기에 가장 가까운 버디 블록 크기를 검색
	qwAlignedSize = kGetBuddyBlockSize(qwSize);
	if(qwAlignedSize == 0){
		return NULL;
	}

	// 남은 메모리가 충분하지 않는 경우, 실패
	if((gs_stDynamicMemory.qwStartAddress + gs_stDynamicMemory.qwUsedSize + qwAlignedSize) > gs_stDynamicMemory.qwEndAddredss){
		return NULL;
	}

	// 버디 블록을 할당하고, 할당된 블록이 속한 블록 리스트의 비트맵 오프셋를 반환
	lOffset = kAllocateBuddyBlock(qwAlignedSize);
	if(lOffset == -1){
		return NULL;
	}

	// 블록 크기가 일치하는 블록 리스트의 인덱스를 검색
	iIndexOfBlockList = kGetBlockListIndexOfMatchSize(qwAlignedSize);

	// 인덱스 영역의 할당된 블록의 바이트 단위 오프셋 위치에 블록 리스트 인덱스를 저장 (메모리를 해제할 때, 블록 리스트 인덱스를 사용하여 할당된 메모리의 크기를 구함)
	qwRelativeAddress = qwAlignedSize * lOffset;
	iSizeArrayOffset = qwRelativeAddress / DYNAMICMEMORY_MIN_SIZE;
	gs_stDynamicMemory.pbAllocatedBlockListIndex[iSizeArrayOffset] = (BYTE)iIndexOfBlockList;

	// 할당된 메모리 크기를 증가
	gs_stDynamicMemory.qwUsedSize += qwAlignedSize;

	// 할당된 메모리의 절대 주소 반환(절대 주소=블록 풀 시작 어드레스+상대 주소)
	return (void*)(gs_stDynamicMemory.qwStartAddress + qwRelativeAddress);
}

static QWORD kGetBuddyBlockSize(QWORD qwSize){
	long i;

	// 파라미터 메모리 크기에 가장 가까운 버디 블록 크기를 검색
	for(i = 0; i < gs_stDynamicMemory.iMaxLevelCount; i++){
		if(qwSize <= (DYNAMICMEMORY_MIN_SIZE << i)){
			return (DYNAMICMEMORY_MIN_SIZE << i);
		}
	}

	return 0;
}

static int kAllocateBuddyBlock(QWORD qwAlignedSize){
	int iBlockListIndex; // 블록 크기가 일치하는 블록 리스트 인덱스(레벨)
	int iFreeOffset;     // 존재하는 블록의 비트맵 오프셋
	int i;
	BOOL bPreviousInterruptFlag;

	// 블록 크기가 일치하는 블록 리스트의 인덱스를 검색
	iBlockListIndex = kGetBlockListIndexOfMatchSize(qwAlignedSize);
	if(iBlockListIndex == -1){
		return -1;
	}

	bPreviousInterruptFlag = kLockForSystemData();

	// 일치하는 블록 리스트부터 최상위 블록 리스트까지 레벨을 올라가면서, 존재 상태의 블록을 찾음
	for(i = iBlockListIndex; i < gs_stDynamicMemory.iMaxLevelCount; i++){

		// 블록 리스트의 비트맵을 확인하여, 존재 상태의 블록을 찾음
		iFreeOffset = kFindFreeBlockInBitmap(i);
		if(iFreeOffset != -1){
			break;
		}
	}

	// 최상위 블록 리스트까지 검색했는데도, 존재 상태의 블록이 없으면 실패
	if(iFreeOffset == -1){
		kUnlockForSystemData(bPreviousInterruptFlag);
		return -1;
	}

	// 찾은 블록을 부재 상태로 설정
	kSetFlagInBitmap(i, iFreeOffset, DYNAMICMEMORY_EMPTY);

	// 상위 블록 리스트에서 블록은 찾은 경우, 상위 블록을 분할
	if(i > iBlockListIndex){

		// 찾은 블록 리스트부터 일치하는 블록 리스트까지 레벨을 내려가면서, 블록을 분할
		for(i = i - 1; i >= iBlockListIndex; i--){
			// 왼쪽 블록을 부재 상태로 설정
			kSetFlagInBitmap(i, iFreeOffset * 2, DYNAMICMEMORY_EMPTY);

			// 오른쪽 블록을 존재 상태로 설정
			kSetFlagInBitmap(i, iFreeOffset * 2 + 1, DYNAMICMEMORY_EXIST);

			// 왼쪽 블록을 다시 분할
			iFreeOffset = iFreeOffset * 2;
		}
	}

	kUnlockForSystemData(bPreviousInterruptFlag);

	return iFreeOffset;
}

static int kGetBlockListIndexOfMatchSize(QWORD qwAlignedSize){
	int i;

	// 블록 크기가 일치하는 블록 리스트의 인덱스를 검색(블록 리스트 인덱스=레벨)
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

	// 비트맵에 존재하는 비트수(1인 비트수)가 0개인 경우, 실패
	if(gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].qwExistBitCount == 0){
		return -1;
	}

	// 블록 리스트의 블록 개수를 구한 후, 그 개수만큼 비트맵을 검색
	iMaxCount = gs_stDynamicMemory.iBlockCountOfSmallestBlock >> iBlockListIndex;
	pbBitmap = gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].pbBitmap;

	for(i = 0; i < iMaxCount;){

		// 블록 개수가 64개 이상인 경우, 64비트(QWORD)를 한꺼번에 검사해서 1인 비트가 있는지 확인
		if(((iMaxCount - i) / 64) > 0){

			pqwBitmap = (QWORD*)&(pbBitmap[i/8]);

			// 64비트가 모두 0인 경우, 모두 제외
			if(*pqwBitmap == 0){
				i += 64;
				continue;
			}
		}

		// 비트맵의 오프셋 위치의 비트가 1인 경우, 해당 오프셋을 반환(존재하는 블록의 비트맵 오프셋을 반환)
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

	// 블록 존재 설정
	if(bFlag == DYNAMICMEMORY_EXIST){
		// 비트맵의 오프셋 위치의 비트가 0->1(블록 부재->블록 존재)로 변경되는 경우, 존재하는 비트수를 증가
		if((pbBitmap[iOffset/8] & (0x01 << (iOffset % 8))) == 0){
			gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].qwExistBitCount++;
		}

		// 비트맵의 오프셋 위치의 비트를 1(블록 존재)로 설정
		pbBitmap[iOffset/8] |= (0x01 << (iOffset % 8));

	// 블록 부재 설정
	}else{
		// 비트맵의 오프셋 위치의 비트가 1->0(블록 존재->블록 부재)으로 변경되는 경우, 존재하는 비트수를 감소
		if((pbBitmap[iOffset/8] & (0x01 << (iOffset % 8))) != 0){
			gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].qwExistBitCount--;
		}

		// 비트맵의 오프셋 위치의 비트를 0(블록 부재)으로 설정
		pbBitmap[iOffset/8] &= ~(0x01 << (iOffset % 8));
	}
}

BOOL kFreeMemory(void* pvAddress){
	QWORD qwRelativeAddress; // 해제할 블록의 상대 주소(블록 풀 시작 어드레스를 기준으로 한 상대 주소)
	int iSizeArrayOffset;    // 해제할 블록의 바이트 단위 오프셋
	QWORD qwBlockSize;       // 해제할 블록의 크기
	int iBlockListIndex;     // 해제할 블록의 블록 리스트 인덱스
	int iBitmapOffset;       // 해제할 블록의 비트맵 오프셋

	if(pvAddress == NULL){
		return FALSE;
	}

	// 해제할 블록의 상대 주소, 해제할 블록의 바이트 단위 오프셋을 계산
	qwRelativeAddress = ((QWORD)pvAddress) - gs_stDynamicMemory.qwStartAddress;
	iSizeArrayOffset = qwRelativeAddress / DYNAMICMEMORY_MIN_SIZE;

	// 할당된 상태가 아닌 경우, 실패
	if(gs_stDynamicMemory.pbAllocatedBlockListIndex[iSizeArrayOffset] == 0xFF){
		return FALSE;
	}

	// 해제할 블록의 블록 리스트 인덱스를 추출한 후, 초기화
	iBlockListIndex = (int)gs_stDynamicMemory.pbAllocatedBlockListIndex[iSizeArrayOffset];
	gs_stDynamicMemory.pbAllocatedBlockListIndex[iSizeArrayOffset] = 0xFF;

	// 해제할 블록의 크기를 계산
	qwBlockSize = DYNAMICMEMORY_MIN_SIZE << iBlockListIndex;

	// 해제할 블록의 블록 리스트 인덱스와 비트맵 오프셋을 이용해서, 버디 블록 해제
	iBitmapOffset = qwRelativeAddress / qwBlockSize;
	if(kFreeBuddyBlock(iBlockListIndex, iBitmapOffset) == TRUE){

		// 할당된 메모리 크기를 감소
		gs_stDynamicMemory.qwUsedSize -= qwBlockSize;
		return TRUE;
	}

	return FALSE;
}

// PARAM1 : iBlockListIndex      -> 해제할 블록의 블록 리스트 인덱스
// PARAM2 : iBlockOffset         -> 해제할 블록의 블록 오프셋(비트맵 오프셋)
static BOOL kFreeBuddyBlock(int iBlockListIndex, int iBlockOffset){
	int iBuddyBlockOffset;       // 인접 블록 오프셋
	int i;                       // 인덱스
	BYTE bFlag;                  // 인접 블록 상태 플래그
	BOOL bPreviousInterruptFlag; // 이전 인터럽트 플래그

	bPreviousInterruptFlag = kLockForSystemData();

	// 해제할 블록 리스트부터 최상위 블록 리스트까지 레벨을 올라가면서, 블록을 결합
	for(i = iBlockListIndex; i < gs_stDynamicMemory.iMaxLevelCount; i++){

		// 현재 블록을 존재 상태로 설정
		kSetFlagInBitmap(i, iBlockOffset, DYNAMICMEMORY_EXIST);

		// 블록 오프셋이 짝수(왼쪽)면 홀수(오른쪽)를 검사하고, 홀수(오른쪽)면 짝수(왼쪽)를 검사하여, 인접한 블록이 존재한다면 결합
		if((iBlockOffset % 2) == 0){
			iBuddyBlockOffset = iBlockOffset + 1;

		}else{
			iBuddyBlockOffset = iBlockOffset - 1;
		}

		// 인접 블록의 상태 플래그 취득
		bFlag = kGetFlagInBitmap(i, iBuddyBlockOffset);

		// 인접 블록이 부재 상태인 경우, 종료
		if(bFlag == DYNAMICMEMORY_EMPTY){
			break;
		}

		// 인접한 블록이 존재 상태인 경우, 블록을 결합(현재 블록과 인접 블록을 모두 부재 상태로 설정하고, 상위 블록 리스트로 이동)
		kSetFlagInBitmap(i, iBuddyBlockOffset, DYNAMICMEMORY_EMPTY);
		kSetFlagInBitmap(i, iBlockOffset, DYNAMICMEMORY_EMPTY);

		// 상위 블록 리스트의 블록 오프셋을 변경하고, 위의 과정을 상위 블록 리스트에서 다시 반복
		iBlockOffset = iBlockOffset / 2;
	}

	kUnlockForSystemData(bPreviousInterruptFlag);
	return TRUE;
}

static BYTE kGetFlagInBitmap(int iBlockListIndex, int iOffset){
	BYTE* pbBitmap;

	pbBitmap = gs_stDynamicMemory.pstBitmapOfLevel[iBlockListIndex].pbBitmap;

	// 비트맵의 오프셋 위치의 비트가 1인 경우, 블록 존재 상태 반환
	if((pbBitmap[iOffset/8] & (0x01 << (iOffset % 8))) != 0){
		return DYNAMICMEMORY_EXIST;
	}

	// 비트맵의 오프셋 위치의 비트가 0인 경우, 블록 부재 상태 반환
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

