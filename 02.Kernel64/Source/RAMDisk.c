#include "RAMDisk.h"
#include "Utility.h"
#include "DynamicMemory.h"

/***** 전역 변수 선언 *****/
static RDDMANAGER gs_stRDDManager; // 램 디스크 매니저

BOOL kInitializeRDD(DWORD dwTotalSectorCount){

	kMemSet(&gs_stRDDManager, 0, sizeof(gs_stRDDManager));

	// 램 디스크용 메모리 할당(8MB)
	gs_stRDDManager.pbBuffer = (BYTE*)kAllocateMemory(dwTotalSectorCount * 512);
	if(gs_stRDDManager.pbBuffer == NULL){
		return FALSE;
	}

	// 총 섹터 수, 뮤텍스 초기화
	gs_stRDDManager.dwTotalSectorCount = dwTotalSectorCount;
	kInitializeMutex(&(gs_stRDDManager.stMutex));

	return TRUE;
}

BOOL kReadRDDInformation(BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation){

	kMemSet(pstHDDInformation, 0, sizeof(HDDINFORMATION));

	// 총 섹터 수, 모델 번호, 시리얼 번호 설정
	pstHDDInformation->dwTotalSectors = gs_stRDDManager.dwTotalSectorCount;
	kMemCpy(pstHDDInformation->vwModelNumber, "MINT RAM Disk v1.0", 18);
	kMemCpy(pstHDDInformation->vwSerialNumber, "0000-0000", 9);

	return TRUE;
}

int kReadRDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer){
	int iRealReadCount; // 실제로 읽은 섹터 수

	// 실제로 읽은 섹터 수 = MIN(램 디스크의 남은 섹터 수, 요청한 섹터 수)
	// 책의 로직이 잘못된 것 같아, 아래와 같이 수정
	//iRealReadCount = MIN(gs_stRDDManager->dwTotalSectorCount - (dwLBA + iSectorCount), iSectorCount);
	iRealReadCount = MIN(gs_stRDDManager.dwTotalSectorCount - dwLBA, iSectorCount);

	// 섹터 읽기(램 디스크에서 버퍼로 데이터 복사)
	kMemCpy(pcBuffer, gs_stRDDManager.pbBuffer + (dwLBA * 512), iRealReadCount * 512);

	// 실제로 읽은 섹터 수를 반환
	return iRealReadCount;
}

int kWriteRDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer){
	int iRealWriteCount; // 실제로 쓴 섹터 수

	// 실제로 쓴 섹터 수 = MIN(램 디스크의 남은 섹터 수, 요청한 섹터 수)
	// 책의 로직이 잘못된 것 같아, 아래와 같이 수정
	//iRealWriteCount = MIN(gs_stRDDManager.dwTotalSectorCount - (dwLBA + iSectorCount), iSectorCount);
	iRealWriteCount = MIN(gs_stRDDManager.dwTotalSectorCount - dwLBA, iSectorCount);

	// 섹터 쓰기(버퍼에서 램 디스크로 데이터 복사)
	kMemCpy(gs_stRDDManager.pbBuffer + (dwLBA * 512), pcBuffer, iRealWriteCount * 512);

	// 실제로 쓴 섹터 수를 반환
	return iRealWriteCount;
}
