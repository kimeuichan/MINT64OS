#include "RAMDisk.h"
#include "Utility.h"
#include "DynamicMemory.h"

/***** ���� ���� ���� *****/
static RDDMANAGER gs_stRDDManager; // �� ��ũ �Ŵ���

BOOL kInitializeRDD(DWORD dwTotalSectorCount){

	kMemSet(&gs_stRDDManager, 0, sizeof(gs_stRDDManager));

	// �� ��ũ�� �޸� �Ҵ�(8MB)
	gs_stRDDManager.pbBuffer = (BYTE*)kAllocateMemory(dwTotalSectorCount * 512);
	if(gs_stRDDManager.pbBuffer == NULL){
		return FALSE;
	}

	// �� ���� ��, ���ؽ� �ʱ�ȭ
	gs_stRDDManager.dwTotalSectorCount = dwTotalSectorCount;
	kInitializeMutex(&(gs_stRDDManager.stMutex));

	return TRUE;
}

BOOL kReadRDDInformation(BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation){

	kMemSet(pstHDDInformation, 0, sizeof(HDDINFORMATION));

	// �� ���� ��, �� ��ȣ, �ø��� ��ȣ ����
	pstHDDInformation->dwTotalSectors = gs_stRDDManager.dwTotalSectorCount;
	kMemCpy(pstHDDInformation->vwModelNumber, "MINT RAM Disk v1.0", 18);
	kMemCpy(pstHDDInformation->vwSerialNumber, "0000-0000", 9);

	return TRUE;
}

int kReadRDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer){
	int iRealReadCount; // ������ ���� ���� ��

	// ������ ���� ���� �� = MIN(�� ��ũ�� ���� ���� ��, ��û�� ���� ��)
	// å�� ������ �߸��� �� ����, �Ʒ��� ���� ����
	//iRealReadCount = MIN(gs_stRDDManager->dwTotalSectorCount - (dwLBA + iSectorCount), iSectorCount);
	iRealReadCount = MIN(gs_stRDDManager.dwTotalSectorCount - dwLBA, iSectorCount);

	// ���� �б�(�� ��ũ���� ���۷� ������ ����)
	kMemCpy(pcBuffer, gs_stRDDManager.pbBuffer + (dwLBA * 512), iRealReadCount * 512);

	// ������ ���� ���� ���� ��ȯ
	return iRealReadCount;
}

int kWriteRDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer){
	int iRealWriteCount; // ������ �� ���� ��

	// ������ �� ���� �� = MIN(�� ��ũ�� ���� ���� ��, ��û�� ���� ��)
	// å�� ������ �߸��� �� ����, �Ʒ��� ���� ����
	//iRealWriteCount = MIN(gs_stRDDManager.dwTotalSectorCount - (dwLBA + iSectorCount), iSectorCount);
	iRealWriteCount = MIN(gs_stRDDManager.dwTotalSectorCount - dwLBA, iSectorCount);

	// ���� ����(���ۿ��� �� ��ũ�� ������ ����)
	kMemCpy(gs_stRDDManager.pbBuffer + (dwLBA * 512), pcBuffer, iRealWriteCount * 512);

	// ������ �� ���� ���� ��ȯ
	return iRealWriteCount;
}
