#ifndef __FILE_SYSTEM_H__
#define __FILE_SYSTEM_H__

#include "Types.h"
#include "Synchronization.h"
#include "HardDisk.h"

#define FILESYSTEM_SIGNATURE		0x7e38cf10
#define FILESYSTEM_SECTORSPERCLUSTER	8
#define FILESYSTEM_LASTCLUSTER			0xffffffff
#define FILESYSTEM_FREECLUSTER			0x00
#define FILESYSTEM_CLUSTERSIZE		(FILESYSTEM_SECTORSPERCLUSTER*512)
#define FILESYSTEM_MAXDIRECTORYENTRYCOUNT	(FILESYSTEM_CLUSTERSIZE / sizeof(DIRECTORYENTRY))
#define FILESYSTEM_MAXFILENAMELENGTH	24

typedef BOOL (*fReadHDDInformation)(BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation);
typedef int (*fReadHDDSector)(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);
typedef int (*fWriteHDDSector)(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);


// MINT File System Architecture
//****************************************************************************************************
// ------------------------------------------------------------------------------------------
// |                    메타 데이터 영역                                               |              일반 데이터 영역                                  |
// ------------------------------------------------------------------------------------------
// | MBR 영역(LBA 0, 1섹터 크기) | 예약된 영역 | 클러스터 링크 테이블 영역 | 루트 디렉토리(클러스터 0, 1클러스터 크기) | 데이터 영역 |
// ------------------------------------------------------------------------------------------
// ==>> MBR 영역(1섹터 크기, 512B) : 부트 로더 코드와 파일 시스템 정보(446B), 파티션 테이블(16B*4=64B), 부트 로더 시그너처(2B)
// ==>> 예약된 영역 : 현재 사용 안 함
// ==>> 클러스터 링크 테이블 영역 : 한 섹터(512B)에 클러스터 링크(4B)를 128개 생성 가능(클러스터 링크 테이블 영역의 크기는 하드 디스크의 전체 크기에 의존함)
// ==>> 루트 디렉토리(1클러스터 크기, 4KB) : 루트 디렉토리(4KB)에 디렉토리 엔트리(32B)를 최대 128개까지 생성 가능(따라서, 파일도 최대 128개까지 생성 가능)
// ==>> 데이터 영역 : 파일 데이터가 존재하는 영역
//****************************************************************************************************

#pragma pack(push, 1)
typedef struct kPartitionStruct{
	// 부트 가능여부 플래그
	BYTE bBootableFlag;
	// CHS 방식 파티션 시작 어드레스(현재는 거의 사용X)
	BYTE vbStartingCHSAddress[3];
	// 파티션 타입(0x00:사용되지 않는 파티션, 0x0C:FAT32 파일 시스템, 0x83:리눅스 파일 시스템)
	BYTE bPartionType;
	// CHS 방식 파티션 끝 어드레스(현재는 거의 사용X)
	BYTE vbEndingCHSAddress[3];
	// 파티션 시작 LBA 어드레스
	DWORD dwStartingLBAAddress;
	// 파티션 섹터 수
	DWORD dwSizeInSector;
} PARTITION; // 16byte

typedef struct kMBRStruct{
	// 부트 로더 코드
	BYTE vbBootCode[430];
	// 파일 시스템 시그니처
	DWORD dwSignature;
	// 예약된 영역 섹터 수
	DWORD dwReservedSectorCount;
	// 클러스터 링크 테이블 섹터 수
	DWORD dwClusterLinkSectorCount;
	// 총 클러스터 수(일반 데이터 영역의 클러스터 수)
	DWORD dwTotalClusterCount;
	// 파티션 테이블
	PARTITION vstPartition[4];
	// 부트로더 시그니처(0x55, 0xaa)
	BYTE vbBootLoaderSignature[2];
} MBR; // 512byte

typedef struct kDirectoryEntryStruct{
	// 파일 이름(확장자 포함, 마지막 NULL 문자 포함)
	char vcFileName[FILESYSTEM_MAXFILENAMELENGTH];
	// 파일 크기
	DWORD dwFileSize;
	// 파일 시작 클러스터 인덱스
	DWORD dwStartClusterIndex;
} DIRECTORYENTRY; // 32byte

typedef struct kFileSystemManagerStruct{
	// 파일 시스템 마운트 여부
	BOOL bMounted;
	// 예약된 영역 섹터 수
	DWORD dwReservedSectorCount;
	// 클러스터 링크 테이블 시작 어드레스
	DWORD dwClusterLinkAreaStartAddress;
	// 클러스터 링크 테이블 크기(섹터 수)
	DWORD dwClusterLinkAreaSize;
	// 일반 데이터 영역 클러스터 시작 주소
	DWORD dwDataAreaStartAddress;
	// 총 클러스터 수(일반 데이터 영역)
	DWORD dwTotalClusterCount;
	// 마지막으로 할당된 클러스터 링크 테이블의 섹터 오프셋
	DWORD dwLastAllocatedClusterLinkSectorOffset;
	// 동기화 객체
	MUTEX stMutex;
} FILESYSTEMMANAGER;

#pragma pack(pop)

// 함수
BOOL kInitializeFileSystem(void);
BOOL kMount(void);
BOOL kFormat(void);
BOOL kGetHDDInformation(HDDINFORMATION* pstInformation);
BOOL kReadClusterLinkTable(DWORD dwOffset, BYTE* pbBuffer);
BOOL kWriteClusterLinkTable(DWORD dwOffset, BYTE* pbBuffer);
BOOL kReadCluster(DWORD dwOffset, BYTE* pbBuffer);
BOOL kWriteCluster(DWORD dwOffset, BYTE* pbBuffer);
DWORD kFindFreeCluster(void);
BOOL kSetClusterLinkData(DWORD dwClusterIndex, DWORD dwData);
BOOL kGetClusterLinkData(DWORD dwClusterIndex, DWORD* dwData);
int kFindFreeDirectoryEntry(void);
BOOL kSetDirectoryEntryData(int iIndex, DIRECTORYENTRY* pstEntry);
BOOL kGetDirectoryEntry(int iIndex, DIRECTORYENTRY* pstEntry);
int kFindDirectoryEntry(const char* pcFileName, DIRECTORYENTRY* pstEntry);
void kGetFileSystemInformation(FILESYSTEMMANAGER* pstManager);


#endif