#include "FileSystem.h"
#include "HardDisk.h"
#include "DynamicMemory.h"
#include "Task.h"
#include "Utility.h"

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
	kMemSet( &gs_stFileSystemManager, 0, sizeof(gs_stFileSystemManager));
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

	// 핸들을 위한 공간 할당
	gs_stFileSystemManager.pstHandlePool = (FILE*)kAllocateMemory(FILESYSTEM_HANDLE_MAXCOUNT * sizeof(FILE));

	// 메모리 할당이 실패하면 하드 디스크가 인식되지 않은 것으로 설정
	if(gs_stFileSystemManager.pstHandlePool == NULL){
		gs_stFileSystemManager.bMounted = FALSE;
		return FALSE;
	}

	// 핸들 풀을 모두 0으로 설정하여 초기화
	kMemSet(gs_stFileSystemManager.pstHandlePool, 0, FILESYSTEM_HANDLE_MAXCOUNT * sizeof(FILE));

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
static BOOL kReadClusterLinkTable(DWORD dwOffset, BYTE* pbBuffer){
	// 클러스터 링크 테이블 영역의 시작 어드레스를 더함
	return gs_pfReadHDDSector(TRUE, TRUE, gs_stFileSystemManager.dwClusterLinkAreaStartAddress + dwOffset, 1, pbBuffer);
}

// 클러스터 리읔 테이블 내의 오프셋에 한 섹터를 씀
static BOOL kWriteClusterLinkTable(DWORD dwOffset, BYTE* pbBuffer){
	return gs_pfWriteHDDSector(TRUE, TRUE, gs_stFileSystemManager.dwClusterLinkAreaStartAddress + dwOffset, 1, pbBuffer);
}

// 데이터 영역의 오프셋에서 한 클러스터를 읽음
static BOOL kReadCluster(DWORD dwOffset, BYTE* pbBuffer){
	return gs_pfReadHDDSector(TRUE, TRUE, gs_stFileSystemManager.dwDataAreaStartAddress + (dwOffset * FILESYSTEM_SECTORSPERCLUSTER), FILESYSTEM_SECTORSPERCLUSTER, pbBuffer);
}

// 데이터 영역의 오프셋에서 한 클러스터를 씀
static BOOL kWriteCluster(DWORD dwOffset, BYTE* pbBuffer){
	return gs_pfWriteHDDSector(TRUE, TRUE, gs_stFileSystemManager.dwDataAreaStartAddress + (dwOffset * FILESYSTEM_SECTORSPERCLUSTER), FILESYSTEM_SECTORSPERCLUSTER, pbBuffer);
}

// 클러스터 링크 테이블에 영역에서 빈 클러스터를 검색
static DWORD kFindFreeCluster(void){
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
static BOOL kSetClusterLinkData(DWORD dwClusterIndex, DWORD dwData){
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
static BOOL kGetClusterLinkData(DWORD dwClusterIndex, DWORD* dwData){
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
static int kFindFreeDirectoryEntry(void){
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
static BOOL kSetDirectoryEntryData(int iIndex, DIRECTORYENTRY* pstEntry){
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
BOOL kGetDirectoryEntryData(int iIndex, DIRECTORYENTRY* pstEntry){
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
static int kFindDirectoryEntry(const char* pcFileName, DIRECTORYENTRY* pstEntry){
	DIRECTORYENTRY* pstRootEntry;
	int i;
	int iLength;

	if(gs_stFileSystemManager.bMounted == FALSE)
		return -1;

	if(kReadCluster(0, gs_vbTempBuffer) == FALSE)
		return -1;

	iLength = kStrLen(pcFileName);

	pstRootEntry = (DIRECTORYENTRY*)gs_vbTempBuffer;
	// 루트 디렉토리 내에서 루프를 돌면서, 파일 이름이 일치하는 엔트리 검색
	for(i=0; i<FILESYSTEM_MAXDIRECTORYENTRYCOUNT; i++){
		if(kMemCmp(pstRootEntry[i].vcFileName, pcFileName, iLength) == 0){
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

// 고수준
static void* kAllocateFileDirectoryHandle(void){
	int i;
	FILE* pstFile;


	pstFile = gs_stFileSystemManager.pstHandlePool;
	// 핸들 풀을 모두 검색하여 비어 있는 핸들을 반환
	for(i=0; i<FILESYSTEM_HANDLE_MAXCOUNT; i++){
		if(pstFile->bType == FILESYSTEM_TYPE_FREE){
			pstFile->bType = FILESYSTEM_TYPE_FILE;
			return pstFile;
		}
		pstFile++;
	}
	return NULL;
}

// 사용한 핸들을 반환
static void kFreeFileDirectoryHandle(FILE* pstFile){
	// 전체 영역을 초기화
	kMemSet(pstFile, 0, sizeof(FILE));
	pstFile->bType = FILESYSTEM_TYPE_FREE;
}

FILE* kOpenFile(const char* pcFileName, const char* pcMode){
	DIRECTORYENTRY stEntry;
	int iDirectoryEntryOffset;
	int iFileNameLength;
	DWORD dwSecondCluster;
	FILE* pstFile;

	// 파일 이름 검사
	iFileNameLength = kStrLen(pcFileName);
	if( (iFileNameLength >(sizeof(stEntry.vcFileName) -1)) || (iFileNameLength == 0))
		return NULL;

	kLock( &(gs_stFileSystemManager.stMutex));

	// 먼저 파일이 존재하는지 확인, 없다면 옵션을 보고 파일 생성
	iDirectoryEntryOffset = kFindDirectoryEntry(pcFileName, &stEntry);
	if(iDirectoryEntryOffset == -1){
		// r 옵션이면 실패
		if(pcMode[0] == 'r'){
			kUnlock( &(gs_stFileSystemManager.stMutex));
			return NULL;
		}

		// 나머지 옵션 파일 생성
		if(kCreateFile(pcFileName, &stEntry, &iDirectoryEntryOffset) == FALSE){
			kUnlock( &(gs_stFileSystemManager.stMutex));
			return NULL;
		}
	}

	// 파일의 내용을 비워야하는 옵션이면 파일에 연결된 클러스터 모두 제거, 파일 크기 0 설정
	else if(pcMode[0] == 'w'){
		// 시작 클러스터를 찾음
		if(kGetClusterLinkData(stEntry.dwStartClusterIndex, &dwSecondCluster) == FALSE){
			kUnlock( &(gs_stFileSystemManager.stMutex));
			return NULL;
		}

		// 시작 클러스터를 마지막 클러스터로 만듬
		if(kSetClusterLinkData(stEntry.dwStartClusterIndex, FILESYSTEM_LASTCLUSTER) == FALSE){
			kUnlock( &(gs_stFileSystemManager.stMutex));
			return NULL;
		}

		// 다음 클러스터부터 마지막 클러스터까지 모두 해제
		if(kFreeClusterUntilEnd(dwSecondCluster) == FALSE){
			kUnlock( &(gs_stFileSystemManager.stMutex));
			return NULL;
		}

		// 파일의 내용이 모두 비워져 있으므로 크기 0 삽입
		stEntry.dwFileSize = 0;
		if(kSetDirectoryEntryData(iDirectoryEntryOffset, &stEntry) == FALSE){
			kUnlock( &(gs_stFileSystemManager.stMutex));
			return NULL;
		}
	}
	// 파일 핸들을 할당 받아 데이터를 설정한 후 반환
	//	파일 핸들을 할당받아 데이터 설정
	pstFile = kAllocateFileDirectoryHandle();
	if(pstFile == NULL){
		kUnlock( &(gs_stFileSystemManager.stMutex));
		return NULL;
	}
	pstFile->bType = FILESYSTEM_TYPE_FILE;
	pstFile->stFileHandle.iDirectoryEntryOffset = iDirectoryEntryOffset;
	pstFile->stFileHandle.dwFileSize = stEntry.dwFileSize;
	pstFile->stFileHandle.dwStartClusterIndex = stEntry.dwStartClusterIndex;
	pstFile->stFileHandle.dwCurrentClusterIndex = stEntry.dwStartClusterIndex;
	pstFile->stFileHandle.dwPreviousClusterIndex = stEntry.dwStartClusterIndex;
	pstFile->stFileHandle.dwCurrentOffset = 0;

	// a 모드
	if(pcMode[0] == 'a')
		kSeekFile(pstFile, 0, FILESYSTEM_SEEK_END);

	kUnlock( &(gs_stFileSystemManager.stMutex));
	return pstFile;
}

static BOOL kCreateFile(const char* pcFileName, DIRECTORYENTRY* pstEntry, int* piDirectoryEntryIndex){
	DWORD dwCluster;

	// 링크 테이블에서 비어있는 클러스터 반환
	dwCluster = kFindFreeCluster();
	// 빈 클러스터를 할당된 클러스터로 설정
	if( (dwCluster == FILESYSTEM_LASTCLUSTER) || (kSetClusterLinkData(dwCluster, FILESYSTEM_LASTCLUSTER) == FALSE))
		return FALSE;

	// 비어있는 디렉토리 엔트리 반환
	*piDirectoryEntryIndex = kFindFreeDirectoryEntry();
	if(*piDirectoryEntryIndex == -1){
		kSetClusterLinkData(dwCluster, FILESYSTEM_FREECLUSTER);
		return FALSE;
	}

	kMemCpy(pstEntry->vcFileName, pcFileName, kStrLen(pcFileName)+1);
	pstEntry->dwStartClusterIndex = dwCluster;
	pstEntry->dwFileSize = 0;

	// 엔트리에 등록
	if( kSetDirectoryEntryData(*piDirectoryEntryIndex, pstEntry) == FALSE){
		// 실패하면 클러스터를 반환
		kSetClusterLinkData(dwCluster, FILESYSTEM_FREECLUSTER);
		return FALSE;
	}

	return TRUE;
}

static BOOL kFreeClusterUntilEnd(DWORD dwClusterIndex){
	DWORD dwCurrentClusterIndex;
	DWORD dwNextClusterIndex;

	// 클러스터 인덱스를 초기화
	dwCurrentClusterIndex = dwClusterIndex;

	while(dwCurrentClusterIndex != FILESYSTEM_LASTCLUSTER){
		// 다음 클러스터의 인덱스를 가져옴
		if(kGetClusterLinkData(dwCurrentClusterIndex, &dwNextClusterIndex) == FALSE)
			return FALSE;

		// 현재 클러스터를 빈것으로 설정
		if(kSetClusterLinkData(dwCurrentClusterIndex, FILESYSTEM_FREECLUSTER) == FALSE)
			return FALSE;

		dwCurrentClusterIndex = dwNextClusterIndex;
	}
	return TRUE;
}

DWORD kReadFile(void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile){
	// 실제로 읽을수 있는 값
	DWORD dwTotalCount;
	// 읽은 값
	DWORD dwReadCount;
	// 클러스터 내 오프셋 값
	DWORD dwOffsetInCluster;
	// 복사할 크기
	DWORD dwCopySize;
	// 파일 핸들러 임시저장 버퍼
	FILEHANDLE* pstFileHandle;
	// 다음 클러스터 인덱스
	DWORD dwNextClusterIndex;

	if( (pstFile == NULL) || (pstFile->bType != FILESYSTEM_TYPE_FILE))
		return 0;

	pstFileHandle = &(pstFile->stFileHandle);

	if((pstFileHandle->dwCurrentOffset == pstFileHandle->dwFileSize) || (pstFileHandle->dwCurrentClusterIndex == FILESYSTEM_LASTCLUSTER))
		return 0;

	// 파일 끝과 읽을 크기 비교해서 작은 값을 총 읽을 값으로 지정
	dwTotalCount = MIN(dwSize * dwCount, pstFileHandle->dwFileSize - pstFileHandle->dwCurrentOffset);

	kLock( &(gs_stFileSystemManager.stMutex));

	dwReadCount = 0;
	while(dwReadCount != dwTotalCount){
		// 클러스터를 읽어서 버퍼에 복사
		// 현재 클러스터를 읽음
		if(kReadCluster(pstFileHandle->dwCurrentClusterIndex, gs_vbTempBuffer) == FALSE)
			break;

		// 클러스터 내에서 파일 포인터가 존재하는 오프셋을 계산
		dwOffsetInCluster = pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE;

		// 여러 클러스터에 걸쳐져 있다면 현재 클러스터에서 남은 만큼 읽고 다음 클러스터로 이동
		// 후자 인자가 작다면 현재 클러스터 내에서 끝이 난다, 전자 인자가 작다면 다음 클러스터에 읽을 값이 있는 것이다.
		dwCopySize = MIN(FILESYSTEM_CLUSTERSIZE - dwOffsetInCluster, dwTotalCount - dwReadCount);
		kMemCpy((char*)pvBuffer+dwReadCount, gs_vbTempBuffer+dwOffsetInCluster, dwCopySize);

		// 읽은 바이트 수와 파일 포인터의 위치를 갱신
		dwReadCount += dwCopySize;
		pstFileHandle->dwCurrentOffset += dwCopySize;

		// 현재 클러스터를 끝까지 다 읽었으면 다음 클러스터로 이동
		if( (pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE) == 0){
			if(kGetClusterLinkData(pstFileHandle->dwCurrentClusterIndex, &dwNextClusterIndex) == FALSE)
				break;

			pstFileHandle->dwPreviousClusterIndex = pstFileHandle->dwCurrentClusterIndex;
			pstFileHandle->dwCurrentClusterIndex = dwNextClusterIndex;
		}
	}

	kUnlock( &(gs_stFileSystemManager.stMutex));

	// 읽은 레코드 수 반환
	return (dwReadCount);
}

DWORD kWriteFile(const void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile){
	// 쓴 레코드 수
	DWORD dwWriteCount;
	// 총 쓸 레코드 수
	DWORD dwTotalCount;
	// 클러스터 내 오프셋 수
	DWORD dwOffsetInCluster;
	// 복사 가능한 데이터 수
	DWORD dwCopySize;
	// 할당 받은 클러스터 인덱스
	DWORD dwAllocatedClusterIndex;
	// 다음 클러스터 인덱스
	DWORD dwNextClusterIndex;
	// 임시 파일 버퍼
	FILEHANDLE* pstFileHandle;

	if((pstFile == NULL) || (pstFile->bType != FILESYSTEM_TYPE_FILE))
		return 0;

	pstFileHandle = &(pstFile->stFileHandle);

	// 총 바이트 수
	dwTotalCount = dwSize * dwCount;

	kLock( &(gs_stFileSystemManager.stMutex));
	dwWriteCount = 0;

	while(dwWriteCount != dwTotalCount){
		// 현재 클러스터가 파일의 끝이면 클러스터를 할당하여 연결
		if(pstFileHandle->dwCurrentClusterIndex == FILESYSTEM_LASTCLUSTER){
			dwAllocatedClusterIndex = kFindFreeCluster();

			if(dwAllocatedClusterIndex == FILESYSTEM_LASTCLUSTER)
				break;

			if(kSetClusterLinkData(dwAllocatedClusterIndex, FILESYSTEM_LASTCLUSTER) == FALSE)
				break;

			if(kSetClusterLinkData(pstFileHandle->dwPreviousClusterIndex, dwAllocatedClusterIndex) == FALSE){
				kSetClusterLinkData(dwAllocatedClusterIndex, FILESYSTEM_FREECLUSTER);
				break;
			}

			// 현재 클러스터를 할당된 것으로 변경
			pstFileHandle->dwCurrentClusterIndex = dwAllocatedClusterIndex;
			// 새로 할당 받았으니 임시 클러스터 버퍼를 0으로 채움
			kMemSet(gs_vbTempBuffer, 0, sizeof(gs_vbTempBuffer));
		}
		// 한클러스터를 채우지 못하면 클러스터를 읽어서 임시 클러스터 버퍼로 복사
		else if( ((pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE) != 0) || ((dwTotalCount - dwWriteCount) < FILESYSTEM_CLUSTERSIZE)){
			// 전체 클러스터를 덮어쓰는 경우가 아니면 부분만 덮어써야 하므로 현재 클러스터를 읽음
			if(kReadCluster(pstFileHandle->dwCurrentClusterIndex, gs_vbTempBuffer) == FALSE)
				break;
		}
		// 클러스터 내에서 파일 포인터가 존재하는 오프셋을 계산
		dwOffsetInCluster = pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE;

		// 복사할 크기 설정
		dwCopySize = MIN(FILESYSTEM_CLUSTERSIZE - dwOffsetInCluster, dwTotalCount - dwWriteCount);

		kMemCpy(gs_vbTempBuffer + dwOffsetInCluster, (char*)pvBuffer + dwWriteCount, dwCopySize);

		if(kWriteCluster(pstFileHandle->dwCurrentClusterIndex, gs_vbTempBuffer) == FALSE)
			break;

		dwWriteCount += dwCopySize;
		pstFileHandle->dwCurrentOffset += dwCopySize;

		// 현재 클러스터의 끝까지 다 썻으면 다음 클러스터로 이동
		if( (pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE) == 0){
			// 현재 클러스터의 링크 데이터를 찾아 다음 클러스터를 얻음
			if(kGetClusterLinkData(pstFileHandle->dwCurrentClusterIndex, &dwNextClusterIndex) == FALSE)
				break;

			pstFileHandle->dwPreviousClusterIndex = pstFileHandle->dwCurrentClusterIndex;
			pstFileHandle->dwCurrentClusterIndex = dwNextClusterIndex;
		}
	}

	if(pstFileHandle->dwFileSize < pstFileHandle->dwCurrentOffset){
		pstFileHandle->dwFileSize = pstFileHandle->dwCurrentOffset;
		kUpdateDirectoryEntry(pstFileHandle);
	}

	kUnlock( &(gs_stFileSystemManager.stMutex));
	return dwWriteCount;
}

static BOOL kUpdateDirectoryEntry(FILEHANDLE* pstFileHandle){
	DIRECTORYENTRY stEntry;

	// 디렉터리 엔트리 검색
	if((pstFileHandle == NULL) || (kGetDirectoryEntryData(pstFileHandle->iDirectoryEntryOffset, &stEntry) == FALSE))
		return FALSE;

	stEntry.dwFileSize = pstFileHandle->dwFileSize;
	stEntry.dwStartClusterIndex = pstFileHandle->dwStartClusterIndex;

	if(kSetDirectoryEntryData(pstFileHandle->iDirectoryEntryOffset, &stEntry) == FALSE)
		return FALSE;

	return TRUE;
}

int kSeekFile(FILE* pstFile, int iOffset, int iOrigin){
	// 진짜 파일 포인터 위치
	DWORD dwRealOffset;
	// 추가할 클러스터 오프셋?
	DWORD dwClusterOffsetToMove;
	// 현재 클러스터에 오프셋
	DWORD dwCurrentClusterOffset;
	// 파일 링크 테이블의 마지막 클러스터 오프셋
	DWORD dwLastClusterOffset;
	DWORD dwMoveCount;
	DWORD i;
	DWORD dwStartClusterIndex;
	DWORD dwPreviousClusterIndex;
	DWORD dwCurrentClusterIndex;
	FILEHANDLE* pstFileHandle;

	// Origin 과 Offset 을 조합하여 파일 시작을 기준으로 파일 포인터 계산
	// 옵션에 따라 실제 위치 계산
	// 음수면 파일의 시작 방향으로 이동하고, 양수면 파일 끝 방향으로 이동
	pstFileHandle = &(pstFile->stFileHandle);
	switch(iOrigin){
		// 파일의 시작을 기준으로 이동
		case FILESYSTEM_SEEK_SET:
			if(iOffset < 0)
				dwRealOffset = 0;
			else
				dwRealOffset = iOffset;
			break;

		// 현재 위치를 기준으로 이동
		case FILESYSTEM_SEEK_CUR:
			// 이동할 오프셋이 음수이고, 현재 파일 포인터의 값보다 크면
			// 더이상 갈수 없으므로 파일의 처음으로 이동
			if( (iOffset <0) && (pstFileHandle->dwCurrentOffset <= (DWORD)-iOffset))
				dwRealOffset = 0;
			else
				dwRealOffset = pstFileHandle->dwCurrentOffset + iOffset;
			break;

		// 파일의 끝을 기준으로 이동
		case FILESYSTEM_SEEK_END:
			if( (iOffset < 0) && (pstFileHandle->dwFileSize <= (DWORD)-iOffset))
				dwRealOffset = 0;
			else
				dwRealOffset = pstFileHandle->dwFileSize + iOffset;
			break;
	}

	// 파일을 구성하는 클러스터의 개수와 현재 파일 포인터의 위치를 고려하여
	// 옮겨질 파일 포인터가 위치하는 클러스터까지 클러스터 링크를 검색

	// 파일의 마지막 클러스터의 오프셋
	dwLastClusterOffset = pstFileHandle->dwFileSize / FILESYSTEM_CLUSTERSIZE;
	// 파일 포인터가 옮겨질 위치의 클러스터 오프셋
	dwClusterOffsetToMove = dwRealOffset / FILESYSTEM_CLUSTERSIZE;
	// 현재 파일 포인터가 있는 클러스터의 오프셋
	dwCurrentClusterOffset = pstFileHandle->dwCurrentOffset / FILESYSTEM_CLUSTERSIZE;

	// 이동하는 클러스터의 위치가 파일의 마지막 클러스터의 오프셋을 넘어서면
	// 현재 클러스터에서 마지막까지 이동한 후 Write 함수를 이용해서 공백으로 나머지를 채움
	if(dwLastClusterOffset < dwClusterOffsetToMove){
		dwMoveCount = dwLastClusterOffset - dwCurrentClusterOffset;
		dwStartClusterIndex = pstFileHandle->dwCurrentClusterIndex;
	}
	// 현재 파일 포인터 오프셋 보다 이동할 클러스터의 위치가 크면
	// 현재 클러스터를 기준으로 차이 만큼만 이동한다.
	else if(dwCurrentClusterOffset <= dwClusterOffsetToMove){
		dwMoveCount = dwClusterOffsetToMove - dwCurrentClusterOffset;
		dwStartClusterIndex = pstFileHandle->dwCurrentClusterIndex;
	}
	// 이동하는 클러스터의 위치가 현재 클러스터의 이전에 있다면, 첫번째 클러스터 부터 이동하면서 검색
	else{
		dwMoveCount = dwClusterOffsetToMove;
		dwStartClusterIndex = pstFileHandle->dwStartClusterIndex;
	}

	kLock( &(gs_stFileSystemManager.stMutex));

	// 클러스터 이동
	dwCurrentClusterIndex = dwStartClusterIndex;
	for(i=0; i<dwMoveCount; i++){
		// 이전 클러스터 값 보관
		dwPreviousClusterIndex = dwCurrentClusterIndex;

		// 다음 클러스터 인덱스 취득
		if(kGetClusterLinkData(dwPreviousClusterIndex, &dwCurrentClusterIndex) == FALSE){
			kUnlock( &(gs_stFileSystemManager.stMutex));
			return -1;
		}
	}

	// 클러스터를 이동했으면 클러스터 정보를 갱신
	if(dwMoveCount >0){
		pstFileHandle->dwPreviousClusterIndex = dwPreviousClusterIndex;
		pstFileHandle->dwCurrentClusterIndex = dwCurrentClusterIndex;
	}
	else if(dwStartClusterIndex == pstFileHandle->dwStartClusterIndex){
		pstFileHandle->dwPreviousClusterIndex = pstFileHandle->dwStartClusterIndex;
		pstFileHandle->dwCurrentClusterIndex = pstFileHandle->dwStartClusterIndex;
	}


	// 이동할 위치가 파일 크기를 넘어선 경우 나머지 부분을 0으로 채운후, 파일 포인터의 현재 위치를 갱신
	if(dwLastClusterOffset < dwClusterOffsetToMove){
		pstFileHandle->dwCurrentOffset = pstFileHandle->dwFileSize;
		kUnlock( &(gs_stFileSystemManager.stMutex));

		// 나머지 부분을 0으로 채워서 파일 크기를 늘림
		if(kWriteZero(pstFile, dwRealOffset - pstFileHandle->dwFileSize) == FALSE)
			return 0;
	}

	pstFileHandle->dwCurrentOffset = dwRealOffset;
	kUnlock( &(gs_stFileSystemManager.stMutex));
	return 0;
}

// 파일을 Count 만큼 0로 채움
BOOL kWriteZero(FILE* pstFile, DWORD dwCount){
	// 0보관 임시 버퍼
	BYTE* pbBuffer;
	// 실제 동작 횟수
	DWORD dwRemainCount;
	// 실제 클러스터에 쓸 수
	DWORD dwWriteCount;

	if(pstFile == NULL)
		return FALSE;

	// 속도 향상을 위해 메모리를 할당받아 클러스터 단위로 쓰기 수행 메모리를 할당
	pbBuffer = (BYTE*)kAllocateMemory(FILESYSTEM_CLUSTERSIZE);
	if(pbBuffer == NULL)
		return FALSE;

	// 0으로 채움
	kMemSet(pbBuffer, 0, FILESYSTEM_CLUSTERSIZE);
	dwRemainCount = dwCount;

	while(dwRemainCount != 0){
		dwWriteCount = MIN(dwRemainCount, FILESYSTEM_CLUSTERSIZE);
		if(kWriteFile(pbBuffer, 1, dwWriteCount, pstFile) == FALSE){
			kFreeMemory(pbBuffer);
			return FALSE;
		}
		dwRemainCount -= dwWriteCount;
	}
	kFreeMemory(pbBuffer);
	return TRUE;
}

int kCloseFile(FILE* pstFile){
	if((pstFile == NULL) || (pstFile->bType != FILESYSTEM_TYPE_FILE))
		return -1;

	kFreeFileDirectoryHandle(pstFile);
	return 0;
}

BOOL kIsFileOpen(const DIRECTORYENTRY* pstEntry){
	int i;
	FILE* pstFile;

	// 핸들 풀의 시작 어드레스부터 끝까지 열린 파일 파일만 검색
	pstFile = gs_stFileSystemManager.pstHandlePool;
	for(i=0; i<FILESYSTEM_HANDLE_MAXCOUNT; i++){
		if( (pstFile[i].bType == FILESYSTEM_TYPE_FILE) && (pstFile[i].stFileHandle.dwStartClusterIndex == pstEntry->dwStartClusterIndex))
			return TRUE;
	}
	return FALSE;
}

int kRemoveFile(const char* pcFileName){
	DIRECTORYENTRY stEntry;
	int iDirectoryEntryOffset;
	int iFileNameLength;

	// 파일 이름 검사
	iFileNameLength = kStrLen(pcFileName);
	if( (iFileNameLength > (sizeof(stEntry.vcFileName) -1) || iFileNameLength == 0))
		return NULL;

	kLock( &(gs_stFileSystemManager.stMutex));

	// 파일 존재 확인
	iDirectoryEntryOffset = kFindDirectoryEntry(pcFileName, &stEntry);
	if(iDirectoryEntryOffset == -1){
		kUnlock( &(gs_stFileSystemManager.stMutex));
		return -1;
	}

	// 다른 태스크에 열려 있으면 파일 삭제x
	if(kIsFileOpen(&stEntry) == TRUE){
		kUnlock( &(gs_stFileSystemManager.stMutex));
		return -1;
	}

	// 파일을 구성하는 클러스터를 모두 해제
	if(kFreeClusterUntilEnd(stEntry.dwStartClusterIndex) == FALSE){
		kUnlock( &(gs_stFileSystemManager.stMutex));
		return -1;
	}

	kMemSet(&stEntry, 0, sizeof(stEntry));
	if(kSetDirectoryEntryData(iDirectoryEntryOffset, &stEntry) == FALSE){
		kUnlock( &(gs_stFileSystemManager.stMutex));
		return -1;
	}
	kUnlock( &(gs_stFileSystemManager.stMutex));
	return 0;
}

DIR* kOpenDirectory(const char* pcDirectoryName){
	DIR* pstDirectory;
	DIRECTORYENTRY* pstDirectoryBuffer;

	kLock( &(gs_stFileSystemManager.stMutex));

	// 루트 디렉토리 밖에 없으니 무시하고 핸들만 받아서 반환
	pstDirectory = kAllocateFileDirectoryHandle();
	if(pstDirectory == NULL){
		kUnlock( &(gs_stFileSystemManager.stMutex));
		return NULL;
	}

	// 루트 디렉터리를 저장할 버퍼를 할당
	pstDirectoryBuffer = (DIRECTORYENTRY*)kAllocateMemory(FILESYSTEM_CLUSTERSIZE);
	if(pstDirectoryBuffer == NULL){
		kFreeFileDirectoryHandle(pstDirectory);
		kUnlock( &(gs_stFileSystemManager.stMutex));
		return NULL;
	}

	// 루트 디렉터리를 읽음
	if(kReadCluster(0, (BYTE*)pstDirectoryBuffer) == FALSE){
		kFreeFileDirectoryHandle(pstDirectory);
		kFreeMemory(pstDirectoryBuffer);
		kUnlock( &(gs_stFileSystemManager.stMutex));
		return NULL;
	}

	pstDirectory->bType = FILESYSTEM_TYPE_DIRECTORY;
	pstDirectory->stDirectoryHandle.iCurrentOffset = 0;
	pstDirectory->stDirectoryHandle.pstDirectoryBuffer = pstDirectoryBuffer;

	kUnlock( &(gs_stFileSystemManager.stMutex));
	return pstDirectory;
}

struct kDirectoryEntryStruct* kReadDirectory(DIR* pstDirectory){
	DIRECTORYHANDLE* pstDirectoryHandle;
	DIRECTORYENTRY* pstEntry;

	if((pstDirectory == NULL) || (pstDirectory->bType != FILESYSTEM_TYPE_DIRECTORY))
		return NULL;

	pstDirectoryHandle = &(pstDirectory->stDirectoryHandle);

	// 오프셋의 범위가 클러스터에 존재하는 최대값을 넘어서면 실패
	if( (pstDirectoryHandle->iCurrentOffset < 0) || (pstDirectoryHandle->iCurrentOffset >= FILESYSTEM_MAXDIRECTORYENTRYCOUNT))
		return NULL;

	kLock( &(gs_stFileSystemManager.stMutex));

	// 루트 디렉터리에 있는 최대 에느리 의 개수만큼 검색
	pstEntry = pstDirectoryHandle->pstDirectoryBuffer;
	while(pstDirectoryHandle->iCurrentOffset < FILESYSTEM_MAXDIRECTORYENTRYCOUNT){
		// 파일 존재하면 해당 디렉터리 엔트리를 반환
		if(pstEntry[pstDirectoryHandle->iCurrentOffset].dwStartClusterIndex != 0){
			kUnlock( &(gs_stFileSystemManager.stMutex));
			return &(pstEntry[pstDirectoryHandle->iCurrentOffset++]);
		}
		pstDirectoryHandle->iCurrentOffset++;
	}

	kUnlock( &(gs_stFileSystemManager.stMutex));
	return NULL;
}

void kRewindDirectory(DIR* pstDirectory){
	DIRECTORYHANDLE* pstDirectoryHandle;

	// 핸들 타입이 디렉터리가 아니면 실패
	if( (pstDirectory == NULL) || (pstDirectory->bType != FILESYSTEM_TYPE_DIRECTORY))
		return;

	pstDirectoryHandle = &(pstDirectory->stDirectoryHandle);

	kLock( &(gs_stFileSystemManager.stMutex));

	pstDirectoryHandle->iCurrentOffset = 0;

	kUnlock( &(gs_stFileSystemManager.stMutex));
}

int kCloseDirectory(DIR* pstDirectory){
	DIRECTORYHANDLE* pstDirectoryHandle;

	if((pstDirectory == NULL) || (pstDirectory->bType != FILESYSTEM_TYPE_DIRECTORY))
		return -1;

	pstDirectoryHandle = &(pstDirectory->stDirectoryHandle);

	kLock( &(gs_stFileSystemManager.stMutex));

	kFreeMemory(pstDirectoryHandle->pstDirectoryBuffer);
	kFreeFileDirectoryHandle(pstDirectory);

	kUnlock( &(gs_stFileSystemManager.stMutex));

	return 0;
}