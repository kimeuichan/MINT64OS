#ifndef __RAM_DISK_H__
#define __RAM_DISK_H__

#include "Types.h"
#include "Synchronization.h"
#include "HardDisk.h"

/***** 매크로 정의 *****/
#define RDD_TOTALSECTORCOUNT (8 * 1024 * 1024 / 512) // 램 디스크의 총 섹터 수(8MB)

/***** 구조체 정의 *****/
#pragma pack(push, 1)

typedef struct kRDDManagerStruct{
	BYTE* pbBuffer;           // 램 디스크용 메모리 버퍼
	DWORD dwTotalSectorCount; // 총 섹터 수
	MUTEX stMutex;            // 뮤텍스
} RDDMANAGER;

#pragma pack(pop)

/***** 함수 정의 *****/
BOOL kInitializeRDD(DWORD dwTotalSectorCount);
BOOL kReadRDDInformation(BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation);
int kReadRDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);
int kWriteRDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);

#endif // __RAM_DISK_H__
