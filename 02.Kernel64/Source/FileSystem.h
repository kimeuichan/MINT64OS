#ifndef __FILE_SYSTEM_H__
#define __FILE_SYSTEM_H__

#include "Types.h"
#include "Synchronization.h"
#include "HardDisk.h"
#include "Task.h"
#include "CacheManager.h"

/***** 매크로 정의 *****/
// 파일 시스템 매크로
#define FILESYSTEM_SIGNATURE              0x7E38CF10          // MINT 파일 시스템 시그너처
#define FILESYSTEM_SECTORSPERCLUSTER      8                   // 클러스터의 섹터 수(8개)
#define FILESYSTEM_LASTCLUSTER            0xFFFFFFFF          // 마지막 클러스터
#define FILESYSTEM_FREECLUSTER            0x00                // 빈 클러스터
#define FILESYSTEM_CLUSTERSIZE            (FILESYSTEM_SECTORSPERCLUSTER * 512)              // 클러스터의 크기(바이트 수, 4KB)
#define FILESYSTEM_MAXDIRECTORYENTRYCOUNT (FILESYSTEM_CLUSTERSIZE / sizeof(DIRECTORYENTRY)) // 루트 디렉토리의 최대 디렉토리 엔트리 개수(128개)
#define FILESYSTEM_HANDLE_MAXCOUNT        (TASK_MAXCOUNT * 3) // 최대 핸들 개수
#define FILESYSTEM_MAXFILENAMELENGTH      24                  // 최대 파일 이름 길이

// 핸들 타입
#define FILESYSTEM_TYPE_FREE      0 // 빈 핸들
#define FILESYSTEM_TYPE_FILE      1 // 파일 핸들
#define FILESYSTEM_TYPE_DIRECTORY 2 // 디렉토리 핸들

// SEEK 옵션
#define FILESYSTEM_SEEK_SET 0 // 파일의 처음 위치
#define FILESYSTEM_SEEK_CUR 1 // 파일 포인터의 현재 위치
#define FILESYSTEM_SEEK_END 2 // 파일의 마지막 위치

/***** 타입 정의 *****/
// 하드 디스크 제어 관련 함수 포인터 정의
typedef BOOL (* fReadHDDInformation)(BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation);
typedef int (* fReadHDDSector)(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);
typedef int (* fWriteHDDSector)(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);

/***** C 표준 입출력 이름으로 재정의한 매크로 *****/
// MINT 파일 시스템 함수명을 C 표준 입출력 함수명으로 재정의
#define fopen     kOpenFile
#define fread     kReadFile
#define fwrite    kWriteFile
#define fseek     kSeekFile
#define fclose    kCloseFile
#define remove    kRemoveFile
#define opendir   kOpenDirectory
#define readdir   kReadDirectory
#define rewinddir kRewindDirectory
#define closedir  kCloseDirectory

// MINT 파일 시스템 매크로명을 C 표준 입출력 매크로명으로 재정의
#define SEEK_SET FILESYSTEM_SEEK_SET
#define SEEK_CUR FILESYSTEM_SEEK_CUR
#define SEEK_END FILESYSTEM_SEEK_END

// MINT 파일 시스템 타입명, 필드명을 C 표준 입출력 타입명으로 재정의
#define size_t DWORD
#define dirent kDirectoryEntryStruct
#define d_name vcFileName

//****************************************************************************************************
// <<MINT 파일 시스템의 구조>>
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

//****************************************************************************************************
// <<클러스터 관리 알고리즘>>
//  MBR 영역   예약된 영역   클러스터 링크 테이블 영역     C0(루트 디렉토리)    C1       C2       C3       C4       C5      ...
// -----------------------------------------------------------------------------------------------------
// |      |       | C0 : 0xFFFFFFFF | FN, FS, C1 |        |        |        |        |        |        |
// |      |       | C1 : C3         |            |        |        |        |        |        |        |
// |      |       | C2 : 0x00       |            |        |        |        |        |        |        |
// | ...  |  (X)  | C3 : C5         |    ...     |  FD1   |        |  FD2   |        |  FD3   |  ...   |
// |      |       | C4 : 0x00       |            |        |        |        |        |        |        |
// |      |       | C5 : 0xFFFFFFFF |            |        |        |        |        |        |        |
// |      |       |      ...        |            |        |        |        |        |        |        |
// -----------------------------------------------------------------------------------------------------
// ==>> C=Cluster, FN=FileName, FS=FileSize, FD=FileData
// ==>> 클러스터 링크 -> 현재 클러스터 인덱스 : 다음 클러스터 인덱스(단, 0xFFFFFFFF=마지막 클러스터, 0x00=빈 클러스터)
//****************************************************************************************************

/***** 구조체 정의 *****/
#pragma pack(push, 1)

typedef struct kPartitionStruct{
	BYTE bBootableFlag;           // [바이트 0]     : 부팅 가능 플래그(0x80:부팅 가능, 0x00:부팅 불가능)
	BYTE vbStartingCHSAddress[3]; // [바이트 1~3]   : 파티션 시작 CHS 어드레스(현재는 거의 사용하지 않고, 아래의 LBA 어드레스를 사용함)
	BYTE bPartitionType;          // [바이트 4]     : 파티션 타입(0x00:사용되지 않는 파티션, 0x0C:FAT32 파일 시스템, 0x83:리눅스 파일 시스템)
	BYTE vbEndingCHSAddress[3];   // [바이트 5~7]   : 파티션 끝 CHS 어드레스(현재는 거의 사용하지 않고, 아래의 LBA 어드레스를 사용함)
	DWORD dwStartingLBAAddress;   // [바이트 8~11]  : 파티션 시작 LBA 어드레스
	DWORD dwSizeInSector;         // [바이트 12~15] : 파티션의 섹터 수
} PARTITION; // 16 byte 크기

typedef struct kMBRStruct{
	BYTE vbBootCode[430];           // 부트 로더 코드
	DWORD dwSignature;              // 파일 시스템 시그너처(0x7E38CF10)
	DWORD dwReservedSectorCount;    // 예약된 영역의 섹터 수
	DWORD dwClusterLinkSectorCount; // 클러스터 링크 테이블 영역의 섹터 수
	DWORD dwTotalClusterCount;      // 총 클러스터 수(일반 데이터 영역의 클러스터 수)
	PARTITION vstPartition[4];      // 파티션 테이블
	BYTE vbBootLoaderSignature[2];  // 부트 로더 시그너처(0x55, 0xAA)
} MBR; // 1섹터 크기(512 byte)

typedef struct kDirectoryEntryStruct{
	char vcFileName[FILESYSTEM_MAXFILENAMELENGTH]; // [바이트 0~23]  : 파일 이름(파일 확장자 포함, 마지막 NULL 문자 포함)
	DWORD dwFileSize;                              // [바이트 24~27] : 파일 크기(바이트 단위)
	DWORD dwStartClusterIndex;                     // [바이트 28~31] : 시작 클러스터 인덱스(0x00:빈 디렉토리 엔트리)
} DIRECTORYENTRY; // 32 byte 크기

typedef struct kFileHandleStruct{
	int iDirectoryEntryOffset;    // 디렉토리 엔트리 오프셋(파일명이 일치하는 디렉토리 엔트리 인덱스)
	DWORD dwFileSize;             // 파일 크기(바이트 단위)
	DWORD dwStartClusterIndex;    // 시작 클러스터 인덱스
	DWORD dwCurrentClusterIndex;  // 현재 클러스터 인덱스(현재 I/O가 수행중인 클러스터 인덱스)
	DWORD dwPreviousClusterIndex; // 이전 클러스터 인덱스
	DWORD dwCurrentOffset;        // 파일 포인터의 현재 위치(바이트 단위)
} FILEHANDLE;

typedef struct kDirectoryHandleStruct{
	DIRECTORYENTRY* pstDirectoryBuffer; // 루트 디렉토리 버퍼(루트 디렉토리를 저장해 둘 버퍼)
	int iCurrentOffset;                 // 디렉토리 포인터의 현재 위치
} DIRECTORYHANDLE;

typedef struct kFileDirectoryHandleStruct{
	BYTE bType;                            // 핸들 타입(빈 핸들, 파일 핸들, 디렉토리 핸들)

	union{
		FILEHANDLE stFileHandle;           // 파일 핸들
		DIRECTORYHANDLE stDirectoryHandle; // 디렉토리 핸들
	};
} FILE, DIR;

typedef struct kFileSystemManagerStruct{
	BOOL bMounted;                                // 파일 시스템 마운트 여부
	DWORD dwReservedSectorCount;                  // 예약된 영역의 섹터 수
	DWORD dwClusterLinkAreaStartAddress;          // 클러스터 링크 테이블 영역의 시작 어드레스(섹터 단위)
	DWORD dwClusterLinkAreaSize;                  // 클러스터 링크 테이블 영역의 크기(섹터 수)
	DWORD dwDataAreaStartAddress;                 // 일반 데이터 영역의 시작 어드레스(섹터 단위)
	DWORD dwTotalClusterCount;                    // 총 클러스터 수(일반 데이터 영역의 클러스터 수)
	DWORD dwLastAllocatedClusterLinkSectorOffset; // 마지막 할당 클러스터 링크 테이블의 섹터 오프셋
	MUTEX stMutex;                                // 뮤텍스 동기화 객체
	FILE* pstHandlePool;                          // 파일/디렉토리 핸들 풀 어드레스
	BOOL bCacheEnable;                            // 캐시 활성화 플래그
} FILESYSTEMMANAGER;

#pragma pack(pop)

/***** 함수 정의 *****/
// 일반 함수
BOOL kInitializeFileSystem(void);
BOOL kFormat(void);
BOOL kMount(void);
BOOL kGetHDDInformation(HDDINFORMATION* pstInformation);

// 저수준 함수(Low Level Function)
static BOOL kReadClusterLinkTable(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kWriteClusterLinkTable(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kReadCluster(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kWriteCluster(DWORD dwOffset, BYTE* pbBuffer);
static DWORD kFindFreeCluster(void);
static BOOL kSetClusterLinkData(DWORD dwClusterIndex, DWORD dwData);
static BOOL kGetClusterLinkData(DWORD dwClusterIndex, DWORD* pdwData);
static int kFindFreeDirectoryEntry(void);
static BOOL kSetDirectoryEntryData(int iIndex, DIRECTORYENTRY* pstEntry);
static BOOL kGetDirectoryEntryData(int iIndex, DIRECTORYENTRY* pstEntry);
static int kFindDirectoryEntry(const char* pcFileName, DIRECTORYENTRY* pstEntry);
void kGetFileSystemInformation(FILESYSTEMMANAGER* pstManager);

// 고수준 함수(High Level Function)
FILE* kOpenFile(const char* pcFileName, const char* pcMode);
DWORD kReadFile(void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile);
DWORD kWriteFile(const void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile);
int kSeekFile(FILE* pstFile, int iOffset, int iOrigin);
int kCloseFile(FILE* pstFile);
int kRemoveFile(const char* pcFileName);
DIR* kOpenDirectory(const char* pcDirectoryName);
struct kDirectoryEntryStruct* kReadDirectory(DIR* pstDirectory);
void kRewindDirectory(DIR* pstDirectory);
int kCloseDirectory(DIR* pstDirectory);
BOOL kWriteZero(FILE* pstFile, DWORD dwCount);
BOOL kIsFileOpen(const DIRECTORYENTRY* pstEntry);
static void* kAllocateFileDirectoryHandle(void);
static void kFreeFileDirectoryHandle(FILE* pstFile);
static BOOL kCreateFile(const char* pcFileName, DIRECTORYENTRY* pstEntry, int* piDirectoryEntryIndex);
static BOOL kFreeClusterUntilEnd(DWORD dwClusterIndex);
static BOOL kUpdateDirectoryEntry(FILEHANDLE* pstFileHandle);

// 캐시 관련 함수
static BOOL kInternalReadClusterLinkTableWithoutCache(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kInternalReadClusterLinkTableWithCache(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kInternalWriteClusterLinkTableWithoutCache(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kInternalWriteClusterLinkTableWithCache(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kInternalReadClusterWithoutCache(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kInternalReadClusterWithCache(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kInternalWriteClusterWithoutCache(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kInternalWriteClusterWithCache(DWORD dwOffset, BYTE* pbBuffer);
static CACHEBUFFER* kAllocateCacheBufferWithFlush(int iCacheTableIndex);
BOOL kFlushFileSystemCache(void);

#endif // __FILE_SYSTEM_H__
