#include "HardDisk.h"
#include "AssemblyUtility.h"
#include "Utility.h"
#include "Console.h"

/***** ���� ���� ����*****/
static HDDMANAGER gs_stHDDManager;

BOOL kInitializeHDD(void){
	// ���ؽ� �ʱ�ȭ
	kInitializeMutex(&(gs_stHDDManager.stMutex));

	// ���ͷ�Ʈ �÷��� �ʱ�ȭ
	gs_stHDDManager.bPrimaryInterruptOccur = FALSE;
	gs_stHDDManager.bSecondaryInterruptOccur = FALSE;

	// ������ ��� �������Ϳ� 0�� �����Ͽ�, ���ͷ�Ʈ�� Ȱ��ȭ
	kOutPortByte(HDD_PORT_PRIMARYBASE + HDD_PORT_INDEX_DIGITALOUTPUT, 0);
	kOutPortByte(HDD_PORT_SECONDARYBASE + HDD_PORT_INDEX_DIGITALOUTPUT, 0);

	// �ϵ� ��ũ ���� ��û
	if(kReadHDDInformation(TRUE, TRUE, &(gs_stHDDManager.stHDDInformation)) == FALSE){
		gs_stHDDManager.bHDDDetected = FALSE;
		gs_stHDDManager.bCanWrite = FALSE;
		return FALSE;
	}

	// �ϵ� ��ũ�� ����� ���, QEMU�� �������� ���� �ϵ� ��ũ�� ���� �����ϵ��� ����
	gs_stHDDManager.bHDDDetected = TRUE;
	if(kMemCmp(gs_stHDDManager.stHDDInformation.vwModelNumber, "QEMU", 4) == 0){
		gs_stHDDManager.bCanWrite = TRUE;

	}else{
		gs_stHDDManager.bCanWrite = FALSE;
	}

	return TRUE;
}

static BYTE kReadHDDStatus(BOOL bPrimary){
	if(bPrimary == TRUE){
		// ù��° PATA ��Ʈ�� ���� �������Ϳ��� ���� ����
		return kInPortByte(HDD_PORT_PRIMARYBASE + HDD_PORT_INDEX_STATUS);
	}

	// �ι�° PATA ��Ʈ�� ���� �������Ϳ��� ���� ����
	return kInPortByte(HDD_PORT_SECONDARYBASE + HDD_PORT_INDEX_STATUS);
}

static BOOL kWaitForHDDNoBusy(BOOL bPrimary){
	QWORD qwStartTickCount;
	BYTE bStatus;

	qwStartTickCount = kGetTickCount();

	// �ϵ� ��ũ�� No Busy ���°� �� ������, �����ð� ���� ���
	while((kGetTickCount() - qwStartTickCount) <= HDD_WAITTIME){
		bStatus = kReadHDDStatus(bPrimary);

		// ���� ���������� BSY(��Ʈ 7)=0 �� ���, TRUE�� ��ȯ
		if((bStatus & HDD_STATUS_BUSY) != HDD_STATUS_BUSY){
			return TRUE;
		}

		kSleep(1);
	}

	return FALSE;
}

static BOOL kWaitForHDDReady(BOOL bPrimary){
	QWORD qwStartTickCount;
	BYTE bStatus;

	qwStartTickCount = kGetTickCount();

	// �ϵ� ��ũ�� Device Ready ���°� �� ������, �����ð� ���� ���
	while((kGetTickCount() - qwStartTickCount) <= HDD_WAITTIME){
		bStatus = kReadHDDStatus(bPrimary);

		// ���� ���������� DRDY(��Ʈ 6)=1 �� ���, TRUE�� ��ȯ
		if((bStatus & HDD_STATUS_READY) == HDD_STATUS_READY){
			return TRUE;
		}

		kSleep(1);
	}

	return FALSE;
}

void kSetHDDInterruptFlag(BOOL bPrimary, BOOL bFlag){
	if(bPrimary == TRUE){
		gs_stHDDManager.bPrimaryInterruptOccur = bFlag;

	}else{
		gs_stHDDManager.bSecondaryInterruptOccur = bFlag;
	}
}

static BOOL kWaitForHDDInterrupt(BOOL bPrimary){
	QWORD qwStartTickCount;

	qwStartTickCount = kGetTickCount();

	// �ϵ� ��ũ�� ���ͷ�Ʈ�� �߻��� ������, �����ð� ���� ���
	while((kGetTickCount() - qwStartTickCount) <= HDD_WAITTIME){
		// ù��° ���ͷ�Ʈ�� �߻��� ���, TRUE�� ��ȯ
		if((bPrimary == TRUE) && (gs_stHDDManager.bPrimaryInterruptOccur == TRUE)){
			return TRUE;

		// �ι�° ���ͷ�Ʈ�� �߻��� ���, TRUE�� ��ȯ
		}else if((bPrimary == FALSE) && (gs_stHDDManager.bSecondaryInterruptOccur == TRUE)){
			return TRUE;
		}
	}

	return FALSE;
}

BOOL kReadHDDInformation(BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation){
	WORD wPortBase;
	QWORD qwLastTickCount;
	BYTE bStatus;
	BYTE bDriveFlag;
	int i;
	WORD wTemp;
	BOOL bWaitResult;

	if(bPrimary == TRUE){
		wPortBase = HDD_PORT_PRIMARYBASE;

	}else{
		wPortBase = HDD_PORT_SECONDARYBASE;
	}

	kLock(&(gs_stHDDManager.stMutex));

	// �ϵ� ��ũ�� �̹� �������� Ŀ�ǵ尡 �ִ� ���, �����ð� ���� ���
	if(kWaitForHDDNoBusy(bPrimary) == FALSE){
		kUnlock(&(gs_stHDDManager.stMutex));
		return FALSE;
	}

	//====================================================================================================
	// ����̺�/��� �������Ϳ� ������(LBA ���, ����̺� ��ȣ)�� �۽�
	//====================================================================================================

	if(bMaster){
		bDriveFlag = HDD_DRIVEANDHEAD_LBA;

	}else{
		bDriveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
	}

	// ����̹�/��� �������Ϳ� �������� �۽�
	kOutPortByte(wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD, bDriveFlag);

	//====================================================================================================
	// ����̺� �ν� Ŀ�ǵ� �۽� ��, ���ͷ�Ʈ ���
	//====================================================================================================

	// �ϵ� ��ũ�� Ŀ�ǵ带 ���� ������ ���°� �� ������, �����ð� ���� ���
	if(kWaitForHDDReady(bPrimary) == FALSE){
		kUnlock(&(gs_stHDDManager.stMutex));
		return FALSE;
	}

	kSetHDDInterruptFlag(bPrimary, FALSE);

	// Ŀ�ǵ� �������Ϳ� ����̺� �ν� Ŀ�ǵ带 �۽�
	kOutPortByte(wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_IDENTIFY);

	// Ŀ�ǵ� ó���� �Ϸ�ǰ� ���ͷ�Ʈ�� �߻��� ������, �����ð� ���� ���
	bWaitResult = kWaitForHDDInterrupt(bPrimary);

	// ���ͷ�Ʈ�� �߻����� �ʾҰų�, ó������ ������ �߻��� ���, ����
	bStatus = kReadHDDStatus(bPrimary);
	if((bWaitResult == FALSE) || ((bStatus & HDD_STATUS_ERROR) == HDD_STATUS_ERROR)){
		kUnlock(&(gs_stHDDManager.stMutex));
		return FALSE;
	}

	//====================================================================================================
	// ������ ����
	//====================================================================================================

	// ������ �������Ϳ��� 1���͸�ŭ�� �����͸� ����
	for(i = 0; i < (512 / 2); i++){
		((WORD*)pstHDDInformation)[i] = kInPortWord(wPortBase + HDD_PORT_INDEX_DATA);
	}

	// ���ڿ��� ����Ʈ ������ ����
	kSwapByteInWord(pstHDDInformation->vwModelNumber, sizeof(pstHDDInformation->vwModelNumber) / 2);
	kSwapByteInWord(pstHDDInformation->vwSerialNumber, sizeof(pstHDDInformation->vwSerialNumber) / 2);

	kUnlock(&(gs_stHDDManager.stMutex));
	return TRUE;
}

static void kSwapByteInWord(WORD* pwData, int iWordCount){
	int i;
	WORD wTemp;

	for(i = 0; i < iWordCount; i++){
		wTemp = pwData[i];
		pwData[i] = (wTemp >> 8) | (wTemp << 8);
	}
}

int kReadHDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer){
	WORD wPortBase;
	int i, j;
	BYTE bDriveFlag;
	BYTE bStatus;
	long lReadCount = 0;
	BOOL bWaitResult;

	// ���� ���� �� ���� �˻�(1~256����)
	if((gs_stHDDManager.bHDDDetected == FALSE) || (iSectorCount <= 0) || (iSectorCount > 256) || ((dwLBA + iSectorCount) >= gs_stHDDManager.stHDDInformation.dwTotalSectors)){
		return 0;
	}

	if(bPrimary == TRUE){
		wPortBase = HDD_PORT_PRIMARYBASE;

	}else{
		wPortBase = HDD_PORT_SECONDARYBASE;
	}

	kLock(&(gs_stHDDManager.stMutex));

	// �ϵ� ��ũ�� �̹� �������� Ŀ�ǵ尡 �ִ� ���, �����ð� ���� ���
	if(kWaitForHDDNoBusy(bPrimary) == FALSE){
		kUnlock(&(gs_stHDDManager.stMutex));
		return FALSE;
	}

	//====================================================================================================
	// ���� �������Ϳ� ���� ���� ��, ���� ��ġ�� �۽��ϰ�, ����̺�/��� �������Ϳ� ������(LBA ���, ����̺� ��ȣ)�� �۽�
	// [����]LBA ��巹��(28��Ʈ)�� [��Ʈ 0~7:���� ��ȣ], [��Ʈ 8~15:�Ǹ��� ��ȣ�� LSB], [��Ʈ 16~23:�Ǹ��� ��ȣ�� MSB], [��Ʈ 24~27:��� ��ȣ ]�� �����
	//====================================================================================================

	// ���� �� �������Ϳ� ���� ���� �۽�
	kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORCOUNT, iSectorCount);

	// ���� ��ȣ �������Ϳ� ���� ��ġ(LBA ��Ʈ 0~7)�� �۽�
	kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORNUMBER, dwLBA);

	// �Ǹ��� LSB �������Ϳ� ���� ��ġ(LBA ��Ʈ 8~15)�� �۽�
	kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERLSB, dwLBA >> 8);

	// �Ǹ��� MSB �������Ϳ� ���� ��ġ(LBA ��Ʈ 16~23)�� �۽�
	kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERMSB, dwLBA >> 16);

	if(bMaster == TRUE){
		bDriveFlag = HDD_DRIVEANDHEAD_LBA;

	}else{
		bDriveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
	}

	// ����̹�/��� �������Ϳ�  ���� ��ġ(LBA ��Ʈ 24~27)�� ������(LBA ���, ����̺� ��ȣ)�� �۽�
	kOutPortByte(wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD, bDriveFlag | ((dwLBA >> 24) & 0x0F));

	//====================================================================================================
	// ���� �б� Ŀ�ǵ� �۽�
	//====================================================================================================

	// �ϵ� ��ũ�� Ŀ�ǵ带 ���� ������ ���°� �� ������, �����ð� ���� ���
	if(kWaitForHDDReady(bPrimary) == FALSE){
		kUnlock(&(gs_stHDDManager.stMutex));
		return FALSE;
	}

	kSetHDDInterruptFlag(bPrimary, FALSE);

	// Ŀ�ǵ� �������Ϳ� ���� �б� Ŀ�ǵ带 �۽�
	kOutPortByte(wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_READ);

	//====================================================================================================
	// ���ͷ�Ʈ ��� ��, ������ ����
	//====================================================================================================

	// ������ �������Ϳ��� ���� ����ŭ�� �����͸� ����
	for(i = 0; i < iSectorCount; i++){
		// ó�����߿� ������ �߻��ϸ�, ����
		bStatus = kReadHDDStatus(bPrimary);
		if((bStatus & HDD_STATUS_ERROR) == HDD_STATUS_ERROR){
			kPrintf("Error Occur in HDD Sector Read\n");
			kUnlock(&(gs_stHDDManager.stMutex));
			return i; // ������ ���� ���� ���� ��ȯ
		}

		// ������ ���� ó���� �Ϸ�� ������, �����ð� ���� ���
		if((bStatus & HDD_STATUS_DATAREQUEST) != HDD_STATUS_DATAREQUEST){

			// ������ ���� ó���� �Ϸ�ǰ� ���ͷ�Ʈ�� �߻��� ������, �����ð� ���� ���
			bWaitResult = kWaitForHDDInterrupt(bPrimary);

			kSetHDDInterruptFlag(bPrimary, FALSE);

			// ���ͷ�Ʈ�� �߻����� ���� ���, ����
			if(bWaitResult == FALSE){
				kPrintf("Interrupt Not Occur in HDD Sector Read\n");
				kUnlock(&(gs_stHDDManager.stMutex));
				return FALSE;
			}
		}

		// ������ �������Ϳ��� 1���͸�ŭ�� �����͸� ����
		for(j = 0; j < (512 / 2); j++){
			((WORD*)pcBuffer)[lReadCount++] = kInPortWord(wPortBase + HDD_PORT_INDEX_DATA);
		}
	}

	kUnlock(&(gs_stHDDManager.stMutex));
	return i; // ������ ���� ���� ���� ��ȯ
}

int kWriteHDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer){
	WORD wPortBase;
	WORD wTemp;
	int i, j;
	BYTE bDriveFlag;
	BYTE bStatus;
	long lWriteCount = 0;
	BOOL bWaitResult;

	// �� ���� �� ���� �˻�(1~256����)
	if((gs_stHDDManager.bCanWrite == FALSE) || (iSectorCount <= 0) || (iSectorCount > 256) || ((dwLBA + iSectorCount) >= gs_stHDDManager.stHDDInformation.dwTotalSectors)){
		return 0;
	}

	if(bPrimary == TRUE){
		wPortBase = HDD_PORT_PRIMARYBASE;

	}else{
		wPortBase = HDD_PORT_SECONDARYBASE;
	}

	// �ϵ� ��ũ�� �̹� �������� Ŀ�ǵ尡 �ִ� ���, �����ð� ���� ���
	if(kWaitForHDDNoBusy(bPrimary) == FALSE){
		return FALSE;
	}

	kLock(&(gs_stHDDManager.stMutex));

	//====================================================================================================
	// ���� �������Ϳ� �� ���� ��, ���� ��ġ�� �۽��ϰ�, ����̺�/��� �������Ϳ� ������(LBA ���, ����̺� ��ȣ)�� �۽�
	// [����]LBA ��巹��(28��Ʈ)�� [��Ʈ 0~7:���� ��ȣ], [��Ʈ 8~15:�Ǹ��� ��ȣ�� LSB], [��Ʈ 16~23:�Ǹ��� ��ȣ�� MSB], [��Ʈ 24~27:��� ��ȣ]�� �����
	//====================================================================================================

	// ���� �� �������Ϳ� ���� ���� �۽�
	kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORCOUNT, iSectorCount);

	// ���� ��ȣ �������Ϳ� ���� ��ġ(LBA ��Ʈ 0~7)�� �۽�
	kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORNUMBER, dwLBA);

	// �Ǹ��� LSB �������Ϳ� ���� ��ġ(LBA ��Ʈ 8~15)�� �۽�
	kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERLSB, dwLBA >> 8);

	// �Ǹ��� MSB �������Ϳ� ���� ��ġ(LBA ��Ʈ 16~23)�� �۽�
	kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERMSB, dwLBA >> 16);

	if(bMaster == TRUE){
		bDriveFlag = HDD_DRIVEANDHEAD_LBA;

	}else{
		bDriveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
	}

	// ����̹�/��� �������Ϳ�  ���� ��ġ(LBA ��Ʈ 24~27)�� ������(LBA ���, ����̺� ��ȣ)�� �۽�
	kOutPortByte(wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD, bDriveFlag | ((dwLBA >> 24) & 0x0F));

	//====================================================================================================
	// ���� ���� Ŀ�ǵ� �۽� ��, ������ �۽��� ������ ���°� �� ������ ���
	//====================================================================================================

	// �ϵ� ��ũ�� Ŀ�ǵ带 ���� ������ ���°� �� ������, �����ð� ���� ���
	if(kWaitForHDDReady(bPrimary) == FALSE){
		kUnlock(&(gs_stHDDManager.stMutex));
		return FALSE;
	}

	// Ŀ�ǵ� �������Ϳ� ���� ���� Ŀ�ǵ带 �۽�
	kOutPortByte(wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_WRITE);

	// ������ �۽��� ������ ���°� �� ������ ���
	while(1){
		bStatus = kReadHDDStatus(bPrimary);

		// ������ �߻��ϸ�, ����
		if((bStatus & HDD_STATUS_ERROR) == HDD_STATUS_ERROR){
			kUnlock(&(gs_stHDDManager.stMutex));
			return 0;
		}

		// ���� ���������� DRQ(��Ʈ 3)=1 �� ���, ������ �۽��� ������ ������
		if((bStatus & HDD_STATUS_DATAREQUEST) == HDD_STATUS_DATAREQUEST){
			break;
		}

		kSleep(1);
	}

	//====================================================================================================
	// ������ �۽� ��, ���ͷ�Ʈ ���
	//====================================================================================================

	// ������ �������Ϳ� ���� ����ŭ�� �����͸� ��
	for(i = 0; i < iSectorCount; i++){

		// ���ͷ�Ʈ �÷��׸� �ʱ�ȭ�ϰ�, ������ �������Ϳ� 1���͸�ŭ�� �����͸� ��
		kSetHDDInterruptFlag(bPrimary, FALSE);
		for(j = 0; j < (512 / 2); j++){
			kOutPortWord(wPortBase + HDD_PORT_INDEX_DATA, ((WORD*)pcBuffer)[lWriteCount++]);
		}

		// ó�����߿� ������ �߻��ϸ�, ����
		bStatus = kReadHDDStatus(bPrimary);
		if((bStatus & HDD_STATUS_ERROR) == HDD_STATUS_ERROR){
			kPrintf("Error Occur in HDD Sector Write\n");
			kUnlock(&(gs_stHDDManager.stMutex));
			return i; // ������ �� ���� ���� ��ȯ
		}

		// ������ �۽� ó���� �Ϸ�� ������, �����ð� ���� ���
		if((bStatus & HDD_STATUS_DATAREQUEST) != HDD_STATUS_DATAREQUEST){

			// ������ �۽� ó���� �Ϸ�ǰ� ���ͷ�Ʈ�� �߻��� ������, �����ð� ���� ���
			bWaitResult = kWaitForHDDInterrupt(bPrimary);

			kSetHDDInterruptFlag(bPrimary, FALSE);

			// ���ͷ�Ʈ�� �߻����� ���� ���, ����
			if(bWaitResult == FALSE){
				kPrintf("Interrupt Not Occur in HDD Sector Write\n");
				kUnlock(&(gs_stHDDManager.stMutex));
				return FALSE;
			}
		}
	}

	kUnlock(&(gs_stHDDManager.stMutex));
	return i; // ������ �� ���� ���� ��ȯ
}

static BOOL kIsHDDBusy(BOOL bPrimary){ // [����]�� �Լ��� å���� �����Ǿ� ���� �ʾƼ�, ���� ���� ��������
	BYTE bStatus;

	bStatus = kReadHDDStatus(bPrimary);

	if((bStatus & HDD_STATUS_BUSY) == HDD_STATUS_BUSY){
		return TRUE;
	}

	return FALSE;
}

static BOOL kIsHDDReady(BOOL bPrimary){ // [����]�� �Լ��� å���� �����Ǿ� ���� �ʾƼ�, ���� ���� ��������
	BYTE bStatus;

	bStatus = kReadHDDStatus(bPrimary);

	if((bStatus & HDD_STATUS_READY) == HDD_STATUS_READY){
		return TRUE;
	}

	return FALSE;
}
