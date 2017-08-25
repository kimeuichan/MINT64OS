#include "FileSystem.h"
#include "HardDisk.h"
#include "DynamicMemory.h"

// 파일 시스템 자료 구조
static FILESYSTEMMANAGER gs_stFileSystemManager;
// 파일 시스템 임시 버퍼
static BYTE gs_vbTempBuffer[FILESYSTEM_SECTORSPERCLUSTER * 512];

// 하드 디스크 제어에 관련된 함수 포인터 선언
fReadHDDInformation gs_pfReadHDDInformation = NULL;
fReadHDDSector gs_pfReadHDDSector = NULL;
fWriteHDDSector gs_pfWriteHDDSector = NULL;

// 파일 시스템 초기화
BOOL kInitializeFileSystem(void){
	// 자료구조 초기화와 동기화 객체 초기화
	kMemSet( gs_stFileSystemManager, 0, sizeof(gs_stFileSystemManager));
	kInitializeMutex( &(gs_stFileSystemManager.stMutex));

	// 하드 디스크를 초기화
	if(kInitializeHDD() == TRUE){
		// 초기화가 성공하면 함수 포인터를 하드 디스크용 함수로 설정
		gs_pfReadHDDInformation = kReadHDDInformation;
		gs_pfReadHDDSector = kReadHDDSector;
		gs_pfWriteHDDSector = kWriteHDDSector;
	}
	else
		return FALSE;

	// 파일 시스템 연결
	if(kMount() == FALSE)
		return FALSE;

	return TRUE;
}

// 저수준(LOW Level Function)
// 하드 디스크의 MBR 읽어서 MINT File System 확인
//	MINT File System 이면 파일 시스템에 관련된 각종 정보를 읽어서 자료 구조에 삽입
BOOL kMount(void){
	MBR* pstMBR;

	// 동기화 처리
	kLock( &(gs_stFileSystemManager.stMutex));

	// MBR을 읽음
	if(gs_pfReadHDDSector(TRUE, TRUE, 0, 1, gs_vbTempBuffer) == FALSE){
		kUnlock( &(gs_stFileSystemManager.stMutex));
		return FALSE;
	}

	// 파일 시스템 시그니처 확인
	pstMBR = (MBR*)gs_vbTempBuffer;
	if(pstMBR->dwSignature != FILESYSTEM_SIGNATURE){
		kUnlock( &(gs_stFileSystemManager.stMutex));
		return FALSE;
	}

	// 파일 시스템 인식 성공
	gs_stFileSystemManager.bMounted = TRUE;

	// 각 영역의 시작 LBA 어드레스와 섹터 수를 설정
	gs_stFileSystemManager.dwReservedSectorCount = pstMBR->dwReservedSectorCount;
	gs_stFileSystemManager.dwClusterLinkAreaStartAddress = 1 + pstMBR->dwReservedSectorCount;
	gs_stFileSystemManager.dwClusterLinkAreaSize = pstMBR->dwClusterLinkSectorCount;
	gs_stFileSystemManager.dwDataAreaStartAddress = 1 + pstMBR->dwReservedSectorCount + pstMBR->dwClusterLinkSectorCount;
	gs_stFileSystemManager.dwTotalClusterCount = pstMBR->dwTotalClusterCount;

	kUnlock( &(gs_stFileSystemManager.stMutex));
	return TRUE;
}

// 하드 디스크에 파일 시스템을 생성
BOOL kFormat(void){
	HDDINFORMATION* pstHDD;
	MBR* pstMBR;
	DWORD dwTotalSectorCount;
	DWORD dwRemainSectorCount;
	DWORD dwMaxClusterCount;
	DWORD dwClusterCount;
	DWORD dwClusterLinkSectorCount;
	DWORD i;

	kLock( &(gs_stFileSystemManager.stMutex));

	// 실제 클러스터수 (일반 데이터 영역의 클러스터 수), 클러스터 링크 테이블 영역의 섹터 수를 구함

	// 하드 디스크 정보를 읽어서, 하드 디스크의총 섹터 수를 구함
	pstHDD = (HDDINFORMATION*)gs_vbTempBuffer;
	if(gs_pfReadHDDInformation(TRUE, TRUE, pstHDD) == FALSE){
		kUnlock( &(gs_stFileSystemManager.stMutex));
		return FALSE;
	}

	dwTotalSectorCount = pstHDD->dwTotalSectors;

	// 최대 클러스터 수 = 하드 디스크의 총 섹터수 / 클러스터의 섹터 수(8)
	dwMaxClusterCount = dwTotalSectorCount / FILESYSTEM_SECTORSPERCLUSTER;

	// 일단은, 최대 클러스터의 수를 기준으로 클러스터 링크 테이블 영역의 섹터 수를 구함
	// [한 섹터(512B)에 클러스터 링크(4B)를 128개 생성 가능, 128 단위로 올림 처리]
	dwClusterLinkSectorCount = (dwMaxClusterCount + 127) / 128;

	// 일반 데이터 영역의 섹터 수 = 하드 하드 디스크의 총 섹터수 - MBR 영역의 섹터수 (1) - 예약된 섹터 수(0) - 클러스터 링크 테이블 영역의 섹터 수
	// 실제 클러스터 수 = 일반 데이터 영역의 섹터 수 / 클러스터의 섹터 수(8)
	dwRemainSectorCount = dwTotalSectorCount - 1 -dwClusterLinkSectorCount;
	dwClusterCount = dwRemainSectorCount / FILESYSTEM_SECTORSPERCLUSTER;

	// 최종적으로, 실제 클러스터 수를 기준으로 클러스터 링크 테이블 영역의 섹터 수를 다시 한번 구함
	dwClusterLinkSectorCount = (dwClusterCount + 127) / 128;

	// MBR 영역 초기화

	// MBR 영역(LBA 0, 1섹터 크기) 읽기
	if(gs_pfReadHDDSector(TRUE, TRUE, 0, 1, gs_vbTempBuffer) == FALSE){
		kUnlock( &(gs_stFileSystemManager.stMutex));
		return FALSE;
	}

	// 파티션 테이블(0), 파일 시스템 정보(위에서 구한 정보를 이용) 초기화
	pstMBR = (MBR*)gs_vbTempBuffer;
	kMemSet(pstMBR->vstPartition, 0, sizeof(pstMBR->vstPartition));
	pstMBR->dwSignature = FILESYSTEM_SIGNATURE;
	pstMBR->dwReservedSectorCount = 0;
	pstMBR->dwClusterLinkSectorCount = dwClusterLinkSectorCount;
	pstMBR->dwTotalClusterCount = dwClusterCount;

	// MBR 영역 쓰기
	if(gs_pfWriteHDDSector(TRUE, TRUE, 0, 1, gs_vbTempBuffer) == FALSE){
		kUnlock( &(gs_stFileSystemManager.stMutex));
		return FALSE;
	}

	// 클러스터 링크 테이블 영역, 루트 디렉토리 초기화
	kMemSet(gs_vbTempBuffer, 0, 512);
	for(i=0; i< (dwClusterLinkSectorCount + FILESYSTEM_SECTORSPERCLUSTER); i++){
		// 단, 클러스터 링크 테이블의 첫번째 클러스터 링크는 루트 디렉토리의 링크를 의미하므로, 할당된 클러스터 링크(마지막 클러스터 링크)로 설정
		if(i == 0)
			((DWORD*)(gs_vbTempBuffer))[0] = FILESYSTEM_LASTCLUSTER;
		else
			((DWORD*)(gs_vbTempBuffer))[0] = FILESYSTEM_FREECLUSTER;

		// LBA 1부터 1섹터씩 씀
		if(gs_pfWriteHDDSector(TRUE, TRUE, i+1, 1, gs_vbTempBuffer) == FALSE){
			kUnlock( &(gs_stFileSystemManager.stMutex));
			return FALSE;
		}
	}

	kUnlock( &(gs_stFileSystemManager.stMutex));
	return TRUE;
}

// 파일 시스템에 연결된 하드 디스크의 정보를 반환
BOOL kGetHDDInformation(HDDINFORMATION* pstInformation){
	BOOL bResult;

	kLock( &(gs_stFileSystemManager.stMutex));

	bResult = gs_pfReadHDDInformation(TRUE, TRUE, pstInformation);

	kUnlock( &(gs_stFileSystemManager.stMutex));
	return bResult;
}

// 클러스터 링크 테이블 내의 오프셋에서 한 섹터를 읽음
BOOL kReadClusterLinkTable(DWORD dwOffset, BYTE* pbBuffer){
	// 클러스터 링크 테이블 영역의 시작 어드레스를 더함
	return gs_pfReadHDDSector(TRUE, TRUE, gs_stFileSystemManager.dwClusterLinkAreaStartAddress + dwOffset, 1, pbBuffer);
}

// 클러스터 리읔 테이블 내의 오프셋에 한 섹터를 씀
BOOL kWriteClusterLinkTable(DWORD dwOffset, BYTE* pbBuffer){
	return gs_pfWriteHDDSector(TRUE, TRUE, gs_stFileSystemManager.dwClusterLinkAreaStartAddress + dwOffset, 1, pbBuffer);
}

// 데이터 영역의 오프셋에서 한 클러스터를 읽음
BOOL kReadCluster(DWORD dwOffset, BYTE* pbBuffer){
	return gs_pfReadHDDSector(TRUE, TRUE, gs_stFileSystemManager.dwClusterLinkAreaStartAddress + (dwOffset * FILESYSTEM_SECTORSPERCLUSTER), 1, pbBuffer);
}

// 데이터 영역의 오프셋에서 한 클러스터를 씀
BOOL kWriteCluster(DWORD dwOffset, BYTE* pbBuffer){
	return gs_pfWriteHDDSector(TRUE, TRUE, gs_stFileSystemManager.dwClusterLinkAreaStartAddress + (dwOffset * FILESYSTEM_SECTORSPERCLUSTER), 1, pbBuffer);
}

// 클러스터 링크 테이블에 영역에서 빈 클러스터를 검색
DWORD kFindFreeCluster(void){
	DWORD dwLinkCountInSector;
	DWORD dwLastSectorOffset, dwCurrentSectorOffset;
	DWORD i, j;

	if(gs_stFileSystemManager.bMounted == FALSE)
		return FILESYSTEM_LASTCLUSTER;

	dwLastSectorOffset = gs_stFileSystemManager.dwLastAllocatedClusterLinkSectorOffset;

	// 마지막 할당 위치부터 루프를 돌면서 빈 클러스터 링크를 검색
	for(i=0; i<gs_stFileSystemManager.dwClusterLinkAreaSize; i++){
		// 클러스터 링크 테이블의 마지막 섹터인 경우, 128개의 클러스터 링크가 존재하지 않을 가능 성이 높음
		if((dwLastSectorOffset + i) == (gs_stFileSystemManager.dwClusterLinkAreaSize - 1))
			// dwLinkCountInSector 는 섹터 내의 링크 정보 갯수
			dwLinkCountInSector = gs_stFileSystemManager.dwTotalClusterCount % 128;
		else
			dwLinkCountInSector = 128;

		// 현재 섹터를 읽음
		dwCurrentSectorOffset = (dwLastSectorOffset + i) % gs_stFileSystemManager.dwClusterLinkAreaSize;
		if(kReadClusterLinkTable(dwCurrentSectorOffset, gs_vbTempBuffer) == FALSE)
			return FILESYSTEM_LASTCLUSTER;

		// 현재 섹터 내에서 빈 클러스터 링크를 검색
		for(j=0; j<dwLinkCountInSector; j++){
			if(((DWORD*)gs_vbTempBuffer)[j] == FILESYSTEM_FREECLUSTER)
				break;
		}

		// 빈 클러스터 링크 검색에 성공한 경우, 클러스터 인덱스를 반환
		if(j != dwLinkCountInSector){
			gs_stFileSystemManager.dwLastAllocatedClusterLinkSectorOffset = dwCurrentSectorOffset;

			return (dwCurrentSectorOffset * 128) + j;
		}
	}
	return FILESYSTEM_LASTCLUSTER;
}

// 클러스터 링크 테이블에 값을 설정
BOOL kSetClusterLinkData(DWORD dwClusterIndex, DWORD dwData){
	DWORD dwSectorOffset;

	if(gs_stFileSystemManager.bMounted == FALSE)
		return FALSE;

	dwSectorOffset = dwClusterIndex / 128;

	if(dwSectorOffset > gs_stFileSystemManager.dwClusterLinkAreaSize)
		return FALSE;

	// 해당 섹터를 읽어서 링크 정보를 반환
	if(kReadClusterLinkTable(dwSectorOffset, gs_vbTempBuffer) == FALSE)
		return FALSE;

	// 해당 클러스터 링크에 데이터를 설정
	((DWORD*)gs_vbTempBuffer)[dwClusterIndex % 128] = dwData;

	// 해당 섹터를 씀
	if(kWriteClusterLinkTable(dwSectorOffset, gs_vbTempBuffer) == FALSE)
		return FALSE;

	return TRUE;
}

// 클러스터 링크 테이블의 값을 반환
BOOL kGetClusterLinkData(DWORD dwClusterIndex, DWORD* dwData){
	DWORD dwSectorOffset;

	if(gs_stFileSystemManager.bMounted == FALSE)
		return FALSE;

	dwSectorOffset = dwClusterIndex / 128;

	if(dwSectorOffset > gs_stFileSystemManager.dwClusterLinkAreaSize)
		return FALSE;

	if(kReadClusterLinkTable(dwSectorOffset, gs_vbTempBuffer) == FALSE)
		return FALSE;

	// 해당 클러스터 링크에 데이터를 설정
	*dwData = ((DWORD*)gs_vbTempBuffer)[dwClusterIndex % 128];

	return TRUE;
}

// 루트 디렉터리에서 빈 디렉터리 엔트리를 반환
int kFindFreeDirectoryEntry(void){
	DIRECTORYENTRY* pstEntry;
	int i;

	if(gs_stFileSystemManager.bMounted == FALSE)
		return -1;

	// 루트 디렉토리(클러스터 0, 1클러스터) 읽기
	if(kReadCluster(0, gs_vbTempBuffer) == FALSE)
		return -1;

	// 루트 디렉토리 내에서 루프를 돌면서, 빈 디렉토리 엔트리(시작 클러스터 인덱스 0x00) 검색
	pstEntry = (DIRECTORYENTRY*)gs_vbTempBuffer;
	for(i=0; i<FILESYSTEM_MAXDIRECTORYENTRYCOUNT; i++){
		if(pstEntry[i].dwStartClusterIndex == 0x00)
			return i;
	}

	return -1;
}

// 루트 디렉터리의 해당 인덱스에 디렉터리 엔트리를 설정
BOOL kSetDirectoryEntryData(int iIndex, DIRECTORYENTRY* pstEntry){
	DIRECTORYENTRY* pstRootEntry;

	if((gs_stFileSystemManager.bMounted == FALSE) || (iIndex < 0) || (iIndex >= FILESYSTEM_MAXDIRECTORYENTRYCOUNT)){
		return FALSE;
	}

	// 루트 디렉토리(클러스터 0) 읽기
	if(kReadCluster(0, gs_vbTempBuffer) == FALSE)
		return FALSE;

	// 루트 디렉토리의 해당 디렉토리 엔드리에 설정
	pstRootEntry = (DIRECTORYENTRY*)gs_vbTempBuffer;
	kMemCpy(pstRootEntry+iIndex, pstEntry, sizeof(DIRECTORYENTRY));

	// 루트 디렉토리(클러스터0) 쓰기
	if(kWriteCluster(0, gs_vbTempBuffer) == FALSE)
		return FALSE;

	return TRUE;
}

// 루트 디렉터리의 해당 인덱스에 위치하는 디렉터리 엔트리를 반환
BOOL kGetDirectoryEntry(int iIndex, DIRECTORYENTRY* pstEntry){
	DIRECTORYENTRY* pstRootEntry;

	if((gs_stFileSystemManager.bMounted == FALSE) || (iIndex < 0) || (iIndex >= FILESYSTEM_MAXDIRECTORYENTRYCOUNT)){
		return FALSE;
	}

	// 루트 디렉토리(클러스터 0) 읽기
	if(kReadCluster(0, gs_vbTempBuffer) == FALSE)
		return FALSE;

	// 루트 디렉토리의 해당 디렉토리 엔드리에 설정
	pstRootEntry = (DIRECTORYENTRY*)gs_vbTempBuffer;
	kMemCpy(pstEntry, pstRootEntry + iIndex, sizeof(DIRECTORYENTRY));

	return TRUE;
}

// 루트 디렉토리에서 파일 이름이 일치하는 엔트리를 찾아서 인덱스를 반환
int kFindDirectoryEntry(const char* pcFileName, DIRECTORYENTRY* pstEntry){
	DIRECTORYENTRY* pstRootEntry;
	int i;
	int iLenght;

	if(gs_stFileSystemManager.bMounted == FALSE)
		return -1;

	if(kReadCluster(0, gs_vbTempBuffer) == FALSE)
		return -1;

	iLenght = kStrLen(pcFileName);

	// 루트 디렉토리 내에서 루프를 돌면서, 파일 이름이 일치하는 엔트리 검색
	for(i=0; i<FILESYSTEM_MAXDIRECTORYENTRYCOUNT; i++){
		if(kMemCmp(pstRootEntry[i].vcFileName, pcFileName, iLenght) == 0){
			kMemCpy(pstEntry, pstRootEntry+i, sizeof(DIRECTORYENTRY));
			return i;
		}
	}
	return -1;
}

// 파일 시스템의 정보를 반환
void kGetFileSystemInformation(FILESYSTEMMANAGER* pstManager){
	kMemCpy(pstManager, &gs_stFileSystemManager, sizeof(gs_stFileSystemManager));
}