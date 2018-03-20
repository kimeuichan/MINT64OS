#include "HardDisk.h"
#include "AssemblyUtility.h"
#include "Utility.h"
#include "Console.h"

/***** 전역 변수 선언*****/
static HDDMANAGER gs_stHDDManager;

BOOL kInitializeHDD(void){
	// 뮤텍스 초기화
	kInitializeMutex(&(gs_stHDDManager.stMutex));

	// 인터럽트 플래그 초기화
	gs_stHDDManager.bPrimaryInterruptOccur = FALSE;
	gs_stHDDManager.bSecondaryInterruptOccur = FALSE;

	// 디지털 출력 레지스터에 0을 설정하여, 인터럽트를 활성화
	kOutPortByte(HDD_PORT_PRIMARYBASE + HDD_PORT_INDEX_DIGITALOUTPUT, 0);
	kOutPortByte(HDD_PORT_SECONDARYBASE + HDD_PORT_INDEX_DIGITALOUTPUT, 0);

	// 하드 디스크 정보 요청
	if(kReadHDDInformation(TRUE, TRUE, &(gs_stHDDManager.stHDDInformation)) == FALSE){
		gs_stHDDManager.bHDDDetected = FALSE;
		gs_stHDDManager.bCanWrite = FALSE;
		return FALSE;
	}

	// 하드 디스크가 검출된 경우, QEMU로 실행했을 때만 하드 디스크에 쓰기 가능하도록 설정
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
		// 첫번째 PATA 포트의 상태 레지스터에서 값을 읽음
		return kInPortByte(HDD_PORT_PRIMARYBASE + HDD_PORT_INDEX_STATUS);
	}

	// 두번째 PATA 포트의 상태 레지스터에서 값을 읽음
	return kInPortByte(HDD_PORT_SECONDARYBASE + HDD_PORT_INDEX_STATUS);
}

static BOOL kWaitForHDDNoBusy(BOOL bPrimary){
	QWORD qwStartTickCount;
	BYTE bStatus;

	qwStartTickCount = kGetTickCount();

	// 하드 디스크가 No Busy 상태가 될 때까지, 일정시간 동안 대기
	while((kGetTickCount() - qwStartTickCount) <= HDD_WAITTIME){
		bStatus = kReadHDDStatus(bPrimary);

		// 상태 레지스터의 BSY(비트 7)=0 인 경우, TRUE를 반환
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

	// 하드 디스크가 Device Ready 상태가 될 때까지, 일정시간 동안 대기
	while((kGetTickCount() - qwStartTickCount) <= HDD_WAITTIME){
		bStatus = kReadHDDStatus(bPrimary);

		// 상태 레지스터의 DRDY(비트 6)=1 인 경우, TRUE를 반환
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

	// 하드 디스크의 인터럽트가 발생할 때까지, 일정시간 동안 대기
	while((kGetTickCount() - qwStartTickCount) <= HDD_WAITTIME){
		// 첫번째 인터럽트가 발생한 경우, TRUE를 반환
		if((bPrimary == TRUE) && (gs_stHDDManager.bPrimaryInterruptOccur == TRUE)){
			return TRUE;

		// 두번째 인터럽트가 발생한 경우, TRUE를 반환
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

	// 하드 디스크가 이미 실행중인 커맨드가 있는 경우, 일정시간 동안 대기
	if(kWaitForHDDNoBusy(bPrimary) == FALSE){
		kUnlock(&(gs_stHDDManager.stMutex));
		return FALSE;
	}

	//====================================================================================================
	// 드라이브/헤드 레지스터에 설정값(LBA 모드, 드라이브 번호)을 송신
	//====================================================================================================

	if(bMaster){
		bDriveFlag = HDD_DRIVEANDHEAD_LBA;

	}else{
		bDriveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
	}

	// 드라이버/헤드 레지스터에 설정값을 송신
	kOutPortByte(wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD, bDriveFlag);

	//====================================================================================================
	// 드라이브 인식 커맨드 송신 후, 인터럽트 대기
	//====================================================================================================

	// 하드 디스크가 커맨드를 수신 가능한 상태가 될 때까지, 일정시간 동안 대기
	if(kWaitForHDDReady(bPrimary) == FALSE){
		kUnlock(&(gs_stHDDManager.stMutex));
		return FALSE;
	}

	kSetHDDInterruptFlag(bPrimary, FALSE);

	// 커맨드 레지스터에 드라이브 인식 커맨드를 송신
	kOutPortByte(wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_IDENTIFY);

	// 커맨드 처리가 완료되고 인터럽트가 발생할 때까지, 일정시간 동안 대기
	bWaitResult = kWaitForHDDInterrupt(bPrimary);

	// 인터럽트가 발생하지 않았거나, 처리도중 에러가 발생한 경우, 종료
	bStatus = kReadHDDStatus(bPrimary);
	if((bWaitResult == FALSE) || ((bStatus & HDD_STATUS_ERROR) == HDD_STATUS_ERROR)){
		kUnlock(&(gs_stHDDManager.stMutex));
		return FALSE;
	}

	//====================================================================================================
	// 데이터 수신
	//====================================================================================================

	// 데이터 레지스터에서 1섹터만큼의 데이터를 읽음
	for(i = 0; i < (512 / 2); i++){
		((WORD*)pstHDDInformation)[i] = kInPortWord(wPortBase + HDD_PORT_INDEX_DATA);
	}

	// 문자열은 바이트 순서를 변경
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

	// 읽을 섹터 수 범위 검사(1~256섹터)
	if((gs_stHDDManager.bHDDDetected == FALSE) || (iSectorCount <= 0) || (iSectorCount > 256) || ((dwLBA + iSectorCount) >= gs_stHDDManager.stHDDInformation.dwTotalSectors)){
		return 0;
	}

	if(bPrimary == TRUE){
		wPortBase = HDD_PORT_PRIMARYBASE;

	}else{
		wPortBase = HDD_PORT_SECONDARYBASE;
	}

	kLock(&(gs_stHDDManager.stMutex));

	// 하드 디스크가 이미 실행중인 커맨드가 있는 경우, 일정시간 동안 대기
	if(kWaitForHDDNoBusy(bPrimary) == FALSE){
		kUnlock(&(gs_stHDDManager.stMutex));
		return FALSE;
	}

	//====================================================================================================
	// 여러 레지스터에 읽을 섹터 수, 섹터 위치를 송신하고, 드라이브/헤드 레지스터에 설정값(LBA 모드, 드라이브 번호)을 송신
	// [참고]LBA 어드레스(28비트)의 [비트 0~7:섹터 번호], [비트 8~15:실리더 번호의 LSB], [비트 16~23:실리더 번호의 MSB], [비트 24~27:헤드 번호 ]가 저장됨
	//====================================================================================================

	// 섹터 수 레지스터에 섹터 수를 송신
	kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORCOUNT, iSectorCount);

	// 섹터 번호 레지스터에 섹터 위치(LBA 비트 0~7)를 송신
	kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORNUMBER, dwLBA);

	// 실리더 LSB 레지스터에 섹터 위치(LBA 비트 8~15)를 송신
	kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERLSB, dwLBA >> 8);

	// 실리더 MSB 레지스터에 섹터 위치(LBA 비트 16~23)를 송신
	kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERMSB, dwLBA >> 16);

	if(bMaster == TRUE){
		bDriveFlag = HDD_DRIVEANDHEAD_LBA;

	}else{
		bDriveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
	}

	// 드라이버/헤드 레지스터에  섹터 위치(LBA 비트 24~27)와 설정값(LBA 모드, 드라이브 번호)를 송신
	kOutPortByte(wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD, bDriveFlag | ((dwLBA >> 24) & 0x0F));

	//====================================================================================================
	// 섹터 읽기 커맨드 송신
	//====================================================================================================

	// 하드 디스크가 커맨드를 수신 가능한 상태가 될 때까지, 일정시간 동안 대기
	if(kWaitForHDDReady(bPrimary) == FALSE){
		kUnlock(&(gs_stHDDManager.stMutex));
		return FALSE;
	}

	kSetHDDInterruptFlag(bPrimary, FALSE);

	// 커맨드 레지스터에 섹터 읽기 커맨드를 송신
	kOutPortByte(wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_READ);

	//====================================================================================================
	// 인터럽트 대기 후, 데이터 수신
	//====================================================================================================

	// 데이터 레지스터에서 섹터 수만큼의 데이터를 읽음
	for(i = 0; i < iSectorCount; i++){
		// 처리도중에 에러가 발생하면, 종료
		bStatus = kReadHDDStatus(bPrimary);
		if((bStatus & HDD_STATUS_ERROR) == HDD_STATUS_ERROR){
			kPrintf("Error Occur in HDD Sector Read\n");
			kUnlock(&(gs_stHDDManager.stMutex));
			return i; // 실제로 읽은 섹터 수를 반환
		}

		// 데이터 수신 처리가 완료될 때까지, 일정시간 동안 대기
		if((bStatus & HDD_STATUS_DATAREQUEST) != HDD_STATUS_DATAREQUEST){

			// 데이터 수신 처리가 완료되고 인터럽트가 발생할 때까지, 일정시간 동안 대기
			bWaitResult = kWaitForHDDInterrupt(bPrimary);

			kSetHDDInterruptFlag(bPrimary, FALSE);

			// 인터럽트가 발생하지 않은 경우, 종료
			if(bWaitResult == FALSE){
				kPrintf("Interrupt Not Occur in HDD Sector Read\n");
				kUnlock(&(gs_stHDDManager.stMutex));
				return FALSE;
			}
		}

		// 데이터 레지스터에서 1섹터만큼의 데이터를 읽음
		for(j = 0; j < (512 / 2); j++){
			((WORD*)pcBuffer)[lReadCount++] = kInPortWord(wPortBase + HDD_PORT_INDEX_DATA);
		}
	}

	kUnlock(&(gs_stHDDManager.stMutex));
	return i; // 실제로 읽은 섹터 수를 반환
}

int kWriteHDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer){
	WORD wPortBase;
	WORD wTemp;
	int i, j;
	BYTE bDriveFlag;
	BYTE bStatus;
	long lWriteCount = 0;
	BOOL bWaitResult;

	// 쓸 섹터 수 범위 검사(1~256섹터)
	if((gs_stHDDManager.bCanWrite == FALSE) || (iSectorCount <= 0) || (iSectorCount > 256) || ((dwLBA + iSectorCount) >= gs_stHDDManager.stHDDInformation.dwTotalSectors)){
		return 0;
	}

	if(bPrimary == TRUE){
		wPortBase = HDD_PORT_PRIMARYBASE;

	}else{
		wPortBase = HDD_PORT_SECONDARYBASE;
	}

	// 하드 디스크가 이미 실행중인 커맨드가 있는 경우, 일정시간 동안 대기
	if(kWaitForHDDNoBusy(bPrimary) == FALSE){
		return FALSE;
	}

	kLock(&(gs_stHDDManager.stMutex));

	//====================================================================================================
	// 여러 레지스터에 쓸 섹터 수, 섹터 위치를 송신하고, 드라이브/헤드 레지스터에 설정값(LBA 모드, 드라이브 번호)을 송신
	// [참고]LBA 어드레스(28비트)의 [비트 0~7:섹터 번호], [비트 8~15:실리더 번호의 LSB], [비트 16~23:실리더 번호의 MSB], [비트 24~27:헤드 번호]가 저장됨
	//====================================================================================================

	// 섹터 수 레지스터에 섹터 수를 송신
	kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORCOUNT, iSectorCount);

	// 섹터 번호 레지스터에 섹터 위치(LBA 비트 0~7)를 송신
	kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORNUMBER, dwLBA);

	// 실리더 LSB 레지스터에 섹터 위치(LBA 비트 8~15)를 송신
	kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERLSB, dwLBA >> 8);

	// 실리더 MSB 레지스터에 섹터 위치(LBA 비트 16~23)를 송신
	kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERMSB, dwLBA >> 16);

	if(bMaster == TRUE){
		bDriveFlag = HDD_DRIVEANDHEAD_LBA;

	}else{
		bDriveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
	}

	// 드라이버/헤드 레지스터에  섹터 위치(LBA 비트 24~27)와 설정값(LBA 모드, 드라이브 번호)를 송신
	kOutPortByte(wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD, bDriveFlag | ((dwLBA >> 24) & 0x0F));

	//====================================================================================================
	// 섹터 쓰기 커맨드 송신 후, 데이터 송신이 가능한 상태가 될 때까지 대기
	//====================================================================================================

	// 하드 디스크가 커맨드를 수신 가능한 상태가 될 때까지, 일정시간 동안 대기
	if(kWaitForHDDReady(bPrimary) == FALSE){
		kUnlock(&(gs_stHDDManager.stMutex));
		return FALSE;
	}

	// 커맨드 레지스터에 섹터 쓰기 커맨드를 송신
	kOutPortByte(wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_WRITE);

	// 데이터 송신이 가능한 상태가 될 때까지 대기
	while(1){
		bStatus = kReadHDDStatus(bPrimary);

		// 에러가 발생하면, 종료
		if((bStatus & HDD_STATUS_ERROR) == HDD_STATUS_ERROR){
			kUnlock(&(gs_stHDDManager.stMutex));
			return 0;
		}

		// 상태 레지스터의 DRQ(비트 3)=1 인 경우, 데이터 송신이 가능한 상태임
		if((bStatus & HDD_STATUS_DATAREQUEST) == HDD_STATUS_DATAREQUEST){
			break;
		}

		kSleep(1);
	}

	//====================================================================================================
	// 데이터 송신 후, 인터럽트 대기
	//====================================================================================================

	// 데이터 레지스터에 섹터 수만큼의 데이터를 씀
	for(i = 0; i < iSectorCount; i++){

		// 인터럽트 플래그를 초기화하고, 데이터 레지스터에 1섹터만큼의 데이터를 씀
		kSetHDDInterruptFlag(bPrimary, FALSE);
		for(j = 0; j < (512 / 2); j++){
			kOutPortWord(wPortBase + HDD_PORT_INDEX_DATA, ((WORD*)pcBuffer)[lWriteCount++]);
		}

		// 처리도중에 에러가 발생하면, 종료
		bStatus = kReadHDDStatus(bPrimary);
		if((bStatus & HDD_STATUS_ERROR) == HDD_STATUS_ERROR){
			kPrintf("Error Occur in HDD Sector Write\n");
			kUnlock(&(gs_stHDDManager.stMutex));
			return i; // 실제로 쓴 섹터 수를 반환
		}

		// 데이터 송신 처리가 완료될 때까지, 일정시간 동안 대기
		if((bStatus & HDD_STATUS_DATAREQUEST) != HDD_STATUS_DATAREQUEST){

			// 데이터 송신 처리가 완료되고 인터럽트가 발생할 때까지, 일정시간 동안 대기
			bWaitResult = kWaitForHDDInterrupt(bPrimary);

			kSetHDDInterruptFlag(bPrimary, FALSE);

			// 인터럽트가 발생하지 않은 경우, 종료
			if(bWaitResult == FALSE){
				kPrintf("Interrupt Not Occur in HDD Sector Write\n");
				kUnlock(&(gs_stHDDManager.stMutex));
				return FALSE;
			}
		}
	}

	kUnlock(&(gs_stHDDManager.stMutex));
	return i; // 실제로 쓴 섹터 수를 반환
}

static BOOL kIsHDDBusy(BOOL bPrimary){ // [주의]이 함수는 책에서 구현되어 있지 않아서, 내가 직접 구현했음
	BYTE bStatus;

	bStatus = kReadHDDStatus(bPrimary);

	if((bStatus & HDD_STATUS_BUSY) == HDD_STATUS_BUSY){
		return TRUE;
	}

	return FALSE;
}

static BOOL kIsHDDReady(BOOL bPrimary){ // [주의]이 함수는 책에서 구현되어 있지 않아서, 내가 직접 구현했음
	BYTE bStatus;

	bStatus = kReadHDDStatus(bPrimary);

	if((bStatus & HDD_STATUS_READY) == HDD_STATUS_READY){
		return TRUE;
	}

	return FALSE;
}
