#ifndef __RAM_DISK_H__
#define __RAM_DISK_H__

#include "Types.h"
#include "Synchronization.h"
#include "HardDisk.h"

/***** ��ũ�� ���� *****/
#define RDD_TOTALSECTORCOUNT (8 * 1024 * 1024 / 512) // �� ��ũ�� �� ���� ��(8MB)

/***** ����ü ���� *****/
#pragma pack(push, 1)

typedef struct kRDDManagerStruct{
	BYTE* pbBuffer;           // �� ��ũ�� �޸� ����
	DWORD dwTotalSectorCount; // �� ���� ��
	MUTEX stMutex;            // ���ؽ�
} RDDMANAGER;

#pragma pack(pop)

/***** �Լ� ���� *****/
BOOL kInitializeRDD(DWORD dwTotalSectorCount);
BOOL kReadRDDInformation(BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation);
int kReadRDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);
int kWriteRDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);

#endif // __RAM_DISK_H__
