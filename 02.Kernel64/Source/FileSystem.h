#ifndef __FILE_SYSTEM_H__
#define __FILE_SYSTEM_H__

#include "Types.h"
#include "Synchronization.h"
#include "HardDisk.h"
#include "Task.h"
#include "CacheManager.h"

/***** ��ũ�� ���� *****/
// ���� �ý��� ��ũ��
#define FILESYSTEM_SIGNATURE              0x7E38CF10          // MINT ���� �ý��� �ñ׳�ó
#define FILESYSTEM_SECTORSPERCLUSTER      8                   // Ŭ�������� ���� ��(8��)
#define FILESYSTEM_LASTCLUSTER            0xFFFFFFFF          // ������ Ŭ������
#define FILESYSTEM_FREECLUSTER            0x00                // �� Ŭ������
#define FILESYSTEM_CLUSTERSIZE            (FILESYSTEM_SECTORSPERCLUSTER * 512)              // Ŭ�������� ũ��(����Ʈ ��, 4KB)
#define FILESYSTEM_MAXDIRECTORYENTRYCOUNT (FILESYSTEM_CLUSTERSIZE / sizeof(DIRECTORYENTRY)) // ��Ʈ ���丮�� �ִ� ���丮 ��Ʈ�� ����(128��)
#define FILESYSTEM_HANDLE_MAXCOUNT        (TASK_MAXCOUNT * 3) // �ִ� �ڵ� ����
#define FILESYSTEM_MAXFILENAMELENGTH      24                  // �ִ� ���� �̸� ����

// �ڵ� Ÿ��
#define FILESYSTEM_TYPE_FREE      0 // �� �ڵ�
#define FILESYSTEM_TYPE_FILE      1 // ���� �ڵ�
#define FILESYSTEM_TYPE_DIRECTORY 2 // ���丮 �ڵ�

// SEEK �ɼ�
#define FILESYSTEM_SEEK_SET 0 // ������ ó�� ��ġ
#define FILESYSTEM_SEEK_CUR 1 // ���� �������� ���� ��ġ
#define FILESYSTEM_SEEK_END 2 // ������ ������ ��ġ

/***** Ÿ�� ���� *****/
// �ϵ� ��ũ ���� ���� �Լ� ������ ����
typedef BOOL (* fReadHDDInformation)(BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation);
typedef int (* fReadHDDSector)(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);
typedef int (* fWriteHDDSector)(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);

/***** C ǥ�� ����� �̸����� �������� ��ũ�� *****/
// MINT ���� �ý��� �Լ����� C ǥ�� ����� �Լ������� ������
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

// MINT ���� �ý��� ��ũ�θ��� C ǥ�� ����� ��ũ�θ����� ������
#define SEEK_SET FILESYSTEM_SEEK_SET
#define SEEK_CUR FILESYSTEM_SEEK_CUR
#define SEEK_END FILESYSTEM_SEEK_END

// MINT ���� �ý��� Ÿ�Ը�, �ʵ���� C ǥ�� ����� Ÿ�Ը����� ������
#define size_t DWORD
#define dirent kDirectoryEntryStruct
#define d_name vcFileName

//****************************************************************************************************
// <<MINT ���� �ý����� ����>>
// ------------------------------------------------------------------------------------------
// |                    ��Ÿ ������ ����                                               |              �Ϲ� ������ ����                                  |
// ------------------------------------------------------------------------------------------
// | MBR ����(LBA 0, 1���� ũ��) | ����� ���� | Ŭ������ ��ũ ���̺� ���� | ��Ʈ ���丮(Ŭ������ 0, 1Ŭ������ ũ��) | ������ ���� |
// ------------------------------------------------------------------------------------------
// ==>> MBR ����(1���� ũ��, 512B) : ��Ʈ �δ� �ڵ�� ���� �ý��� ����(446B), ��Ƽ�� ���̺�(16B*4=64B), ��Ʈ �δ� �ñ׳�ó(2B)
// ==>> ����� ���� : ���� ��� �� ��
// ==>> Ŭ������ ��ũ ���̺� ���� : �� ����(512B)�� Ŭ������ ��ũ(4B)�� 128�� ���� ����(Ŭ������ ��ũ ���̺� ������ ũ��� �ϵ� ��ũ�� ��ü ũ�⿡ ������)
// ==>> ��Ʈ ���丮(1Ŭ������ ũ��, 4KB) : ��Ʈ ���丮(4KB)�� ���丮 ��Ʈ��(32B)�� �ִ� 128������ ���� ����(����, ���ϵ� �ִ� 128������ ���� ����)
// ==>> ������ ���� : ���� �����Ͱ� �����ϴ� ����
//****************************************************************************************************

//****************************************************************************************************
// <<Ŭ������ ���� �˰���>>
//  MBR ����   ����� ����   Ŭ������ ��ũ ���̺� ����     C0(��Ʈ ���丮)    C1       C2       C3       C4       C5      ...
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
// ==>> Ŭ������ ��ũ -> ���� Ŭ������ �ε��� : ���� Ŭ������ �ε���(��, 0xFFFFFFFF=������ Ŭ������, 0x00=�� Ŭ������)
//****************************************************************************************************

/***** ����ü ���� *****/
#pragma pack(push, 1)

typedef struct kPartitionStruct{
	BYTE bBootableFlag;           // [����Ʈ 0]     : ���� ���� �÷���(0x80:���� ����, 0x00:���� �Ұ���)
	BYTE vbStartingCHSAddress[3]; // [����Ʈ 1~3]   : ��Ƽ�� ���� CHS ��巹��(����� ���� ������� �ʰ�, �Ʒ��� LBA ��巹���� �����)
	BYTE bPartitionType;          // [����Ʈ 4]     : ��Ƽ�� Ÿ��(0x00:������ �ʴ� ��Ƽ��, 0x0C:FAT32 ���� �ý���, 0x83:������ ���� �ý���)
	BYTE vbEndingCHSAddress[3];   // [����Ʈ 5~7]   : ��Ƽ�� �� CHS ��巹��(����� ���� ������� �ʰ�, �Ʒ��� LBA ��巹���� �����)
	DWORD dwStartingLBAAddress;   // [����Ʈ 8~11]  : ��Ƽ�� ���� LBA ��巹��
	DWORD dwSizeInSector;         // [����Ʈ 12~15] : ��Ƽ���� ���� ��
} PARTITION; // 16 byte ũ��

typedef struct kMBRStruct{
	BYTE vbBootCode[430];           // ��Ʈ �δ� �ڵ�
	DWORD dwSignature;              // ���� �ý��� �ñ׳�ó(0x7E38CF10)
	DWORD dwReservedSectorCount;    // ����� ������ ���� ��
	DWORD dwClusterLinkSectorCount; // Ŭ������ ��ũ ���̺� ������ ���� ��
	DWORD dwTotalClusterCount;      // �� Ŭ������ ��(�Ϲ� ������ ������ Ŭ������ ��)
	PARTITION vstPartition[4];      // ��Ƽ�� ���̺�
	BYTE vbBootLoaderSignature[2];  // ��Ʈ �δ� �ñ׳�ó(0x55, 0xAA)
} MBR; // 1���� ũ��(512 byte)

typedef struct kDirectoryEntryStruct{
	char vcFileName[FILESYSTEM_MAXFILENAMELENGTH]; // [����Ʈ 0~23]  : ���� �̸�(���� Ȯ���� ����, ������ NULL ���� ����)
	DWORD dwFileSize;                              // [����Ʈ 24~27] : ���� ũ��(����Ʈ ����)
	DWORD dwStartClusterIndex;                     // [����Ʈ 28~31] : ���� Ŭ������ �ε���(0x00:�� ���丮 ��Ʈ��)
} DIRECTORYENTRY; // 32 byte ũ��

typedef struct kFileHandleStruct{
	int iDirectoryEntryOffset;    // ���丮 ��Ʈ�� ������(���ϸ��� ��ġ�ϴ� ���丮 ��Ʈ�� �ε���)
	DWORD dwFileSize;             // ���� ũ��(����Ʈ ����)
	DWORD dwStartClusterIndex;    // ���� Ŭ������ �ε���
	DWORD dwCurrentClusterIndex;  // ���� Ŭ������ �ε���(���� I/O�� �������� Ŭ������ �ε���)
	DWORD dwPreviousClusterIndex; // ���� Ŭ������ �ε���
	DWORD dwCurrentOffset;        // ���� �������� ���� ��ġ(����Ʈ ����)
} FILEHANDLE;

typedef struct kDirectoryHandleStruct{
	DIRECTORYENTRY* pstDirectoryBuffer; // ��Ʈ ���丮 ����(��Ʈ ���丮�� ������ �� ����)
	int iCurrentOffset;                 // ���丮 �������� ���� ��ġ
} DIRECTORYHANDLE;

typedef struct kFileDirectoryHandleStruct{
	BYTE bType;                            // �ڵ� Ÿ��(�� �ڵ�, ���� �ڵ�, ���丮 �ڵ�)

	union{
		FILEHANDLE stFileHandle;           // ���� �ڵ�
		DIRECTORYHANDLE stDirectoryHandle; // ���丮 �ڵ�
	};
} FILE, DIR;

typedef struct kFileSystemManagerStruct{
	BOOL bMounted;                                // ���� �ý��� ����Ʈ ����
	DWORD dwReservedSectorCount;                  // ����� ������ ���� ��
	DWORD dwClusterLinkAreaStartAddress;          // Ŭ������ ��ũ ���̺� ������ ���� ��巹��(���� ����)
	DWORD dwClusterLinkAreaSize;                  // Ŭ������ ��ũ ���̺� ������ ũ��(���� ��)
	DWORD dwDataAreaStartAddress;                 // �Ϲ� ������ ������ ���� ��巹��(���� ����)
	DWORD dwTotalClusterCount;                    // �� Ŭ������ ��(�Ϲ� ������ ������ Ŭ������ ��)
	DWORD dwLastAllocatedClusterLinkSectorOffset; // ������ �Ҵ� Ŭ������ ��ũ ���̺��� ���� ������
	MUTEX stMutex;                                // ���ؽ� ����ȭ ��ü
	FILE* pstHandlePool;                          // ����/���丮 �ڵ� Ǯ ��巹��
	BOOL bCacheEnable;                            // ĳ�� Ȱ��ȭ �÷���
} FILESYSTEMMANAGER;

#pragma pack(pop)

/***** �Լ� ���� *****/
// �Ϲ� �Լ�
BOOL kInitializeFileSystem(void);
BOOL kFormat(void);
BOOL kMount(void);
BOOL kGetHDDInformation(HDDINFORMATION* pstInformation);

// ������ �Լ�(Low Level Function)
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

// ����� �Լ�(High Level Function)
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

// ĳ�� ���� �Լ�
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
