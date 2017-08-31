#ifndef __FILE_SYSTEM_H__
#define __FILE_SYSTEM_H__

#include "Types.h"
#include "Synchronization.h"
#include "HardDisk.h"
#include "Task.h"

#define FILESYSTEM_SIGNATURE		0x7e38cf10
#define FILESYSTEM_SECTORSPERCLUSTER	8
#define FILESYSTEM_LASTCLUSTER			0xffffffff
#define FILESYSTEM_FREECLUSTER			0x00
#define FILESYSTEM_CLUSTERSIZE		(FILESYSTEM_SECTORSPERCLUSTER*512)
#define FILESYSTEM_HANDLE_MAXCOUNT	(TASK_MAXCOUNT * 3)
#define FILESYSTEM_MAXDIRECTORYENTRYCOUNT	(FILESYSTEM_CLUSTERSIZE / sizeof(DIRECTORYENTRY))
#define FILESYSTEM_MAXFILENAMELENGTH	24



// 핸들 타입 정의
#define FILESYSTEM_TYPE_FREE	0
#define FILESYSTEM_TYPE_FILE	1
#define FILESYSTEM_TYPE_DIRECTORY	2

// seek 옵션 정의
#define FILESYSTEM_SEEK_SET			0
#define FILESYSTEM_SEEK_CUR			1
#define FILESYSTEM_SEEK_END			2

typedef BOOL (*fReadHDDInformation)(BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation);
typedef int (*fReadHDDSector)(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);
typedef int (*fWriteHDDSector)(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);



// 표준 형태 재정의
#define fopen 			kOpenFile
#define fread 			kReadFile
#define fwrite 			kWriteFile
#define fseek 			kSeekFile
#define fclose 			kCloseFile
#define remove 			kRemoveFile
#define opendir 		kOpenDirectory
#define readdir 		kReadDirectory
#define rewinddir 		kRewindDirectory
#define closedir 		kCloseDirectory

// MINT 파일 시스템 매크로를 표준 형식화
#define SEEK_SET 		FILESYSTEM_SEEK_SET
#define SEEK_CUR 		FILESYSTEM_SEEK_CUR
#define SEEK_END 		FILESYSTEM_SEEK_END

// MINT 파일 시스템 타입과 필드를 표준 입출력의 타입으로 정의
#define size_t 	DWORD
#define dirent 	kDirectoryEntryStruct
#define d_name 	vcFileName



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

typedef struct kFileHandleStruct{
	// 파일이 존재하는 디렉터리 엔트리 오프셋
	int iDirectoryEntryOffset;
	// 파일 크키
	DWORD dwFileSize;
	// 시작 클러스터 인덱스
	DWORD dwStartClusterIndex;
	// 현재 클러스터 인덱스
	DWORD dwCurrentClusterIndex;
	// 이전 클러스터 인덱스
	DWORD dwPreviousClusterIndex;
	// 파일 포인터 현재 위치
	DWORD dwCurrentOffset;
} FILEHANDLE;

typedef struct kDirectoryHandleStruct{
	// 루트 디렉터리를 저장해둔 버퍼
	DIRECTORYENTRY* pstDirectoryBuffer;
	// 디렉터리 포인터의 현재 위치
	int iCurrentOffset;
} DIRECTORYHANDLE;

typedef struct kFileDirectoryHandleStruct{
	// 자료구조의 타입, (파일이냐 디렉토리냐)
	BYTE bType;
	union {
		FILEHANDLE stFileHandle;
		DIRECTORYHANDLE stDirectoryHandle;
	};
} FILE, DIR;

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

	// 핸들 풀의 어드레스
	FILE* pstHandlePool;
} FILESYSTEMMANAGER;

#pragma pack(pop)

// 함수
BOOL kInitializeFileSystem(void);
BOOL kFormat(void);
BOOL kMount(void);
BOOL kGetHDDInformation(HDDINFORMATION* pstInformation);
// 저수준
static BOOL kReadClusterLinkTable(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kWriteClusterLinkTable(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kReadCluster(DWORD dwOffset, BYTE* pbBuffer);
static BOOL kWriteCluster(DWORD dwOffset, BYTE* pbBuffer);
static DWORD kFindFreeCluster(void);
static BOOL kSetClusterLinkData(DWORD dwClusterIndex, DWORD dwData);
static BOOL kGetClusterLinkData(DWORD dwClusterIndex, DWORD* dwData);
static int kFindFreeDirectoryEntry(void);
static BOOL kSetDirectoryEntryData(int iIndex, DIRECTORYENTRY* pstEntry);
static BOOL kGetDirectoryEntryData(int iIndex, DIRECTORYENTRY* pstEntry);
static int kFindDirectoryEntry(const char* pcFileName, DIRECTORYENTRY* pstEntry);
void kGetFileSystemInformation(FILESYSTEMMANAGER* pstManager);

// 고수준
FILE* kOpenFile(const char* pcFileName, const char* pcMode);
DWORD kReadFile(void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile);
DWORD kWriteFile(const void* pvBuffer, DWORD dwSize, DWORD dwCount, FILE* pstFile);
int kSeekFile(FILE* pstFile, int iOffset, int iOrigin);
int kCloseFile(FILE* pstFile);
int kRemoveFile(const char* pcFileName);
DIR* kOpenDirectory(const char* pcDirectoryName);
struct kDirectoryEntryStruct* kReadDirectory(DIR* pstDirectory);
void kRewinDirectory(DIR* pstDirectory);
int kCloseDirectory(DIR* pstDirectory);
BOOL kWriteZero(FILE* pstFile, DWORD dwCount);
BOOL kIsFileOpen(const DIRECTORYENTRY* pstEntry);
static void* kAllocateFileDirectoryHandle(void);
static void kFreeFileDirectoryHandle(FILE* pstFile);
static BOOL kCreateFile(const char* pcFileName, DIRECTORYENTRY* pstEntry, int* piDirectoryEntryIndex);
static BOOL kFreeClusterUntilEnd(DWORD dwClusterIndex);
static BOOL kUpdateDirectoryEntry(FILEHANDLE* pstFileHandle);

#endif