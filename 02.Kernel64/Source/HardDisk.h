#ifndef __HARDDISK_H__
#define __HARDDISK_H__

#include "Types.h"
#include "synchronization.h"

// 매크로
// 첫번째 PATA 포트와 두번째 PATA 포트
#define HDD_PORT_PRIMARYBASE	0x1f0
#define HDD_PORT_SECONDARYBASE	0x170

// 포트 인덱스에 관련된 매크로
#define HDD_PORT_INDEX_DATA			0x00
#define HDD_PORT_INDEX_SECTORCOUNT	0x02
#define HDD_PORT_INDEX_SECTORNUMBER	0x03
#define HDD_PORT_INDEX_CYLINDERLSB	0x04
#define HDD_PORT_INDEX_CYLINDERMSB	0x05
#define HDD_PORT_INDEX_DRIVEANDHEAD	0x06
#define HDD_PORT_INDEX_STATUS		0x07
#define HDD_PORT_INDEX_COMMAND		0x07
#define HDD_PORT_INDEX_DIGITALOUTPUT	0x206

// 커맨드 레지스터에 관련된 매크로
#define HDD_COMMAND_READ		0x20
#define HDD_COMMAND_WRITE		0x30
#define HDD_COMMAND_IDENTIFY	0xec

// 상태 레지스터에 관련된 매크로
#define HDD_STATUS_ERROR			0x01
#define HDD_STATUS_INDEX			0x02
#define HDD_STATUS_CORRECTEDDATA	0x04
#define HDD_STATUS_DATAREQUEST		0x08
#define HDD_STATUS_SEEKCOMPLETE		0x10
#define HDD_STATUS_WRITEFAULT		0x20
#define HDD_STATUS_READY			0x40
#define HDD_STATUS_BUSY				0x80

// 디바이스/헤드 레지스터에 관련된 매크로
#define HDD_DRIVEANDHEAD_LBA		0xe0
#define HDD_DRIVEANDHEAD_SLAVE		0x01

// 디지털 출력 레지스터에 관련된 매크로
#define HDD_DIGITALOUTPUT_RESET		0x04
#define HDD_DIGITALOUTPUT_DISABLEINTERRUPT	0x01

// 하드 디스크의 응답을 기다리는 시간(ms)
#define HDD_WAITTIME	500
// 한번에 HDD 읽거나 쓸 수 있는 섹터수
#define HDD_MAXBULKSECTORCOUNT		256

#pragma pack(push, 1)

typedef struct kHDDInformationStruct{
	// 설정 값
	WORD wConfiguation;

	// 실린더 수
	WORD wNumberOfCylinder;
	WORD wReserved1;

	// 헤드 수
	WORD wNumberOfHead;
	WORD wUnformattedBytesPerTrak;
	WORD wUnformattedBytesPerSector;

	// 실린더당 섹터 수
	WORD wNumberOfSectorPerCylinder;
	WORD wInterSectorGap;
	WORD wBytesInPhaseLock;
	WORD wNumberOfVendorUniqueStatusWord;

	// 하드 디스크의 시리얼 넘버
	WORD vwSerialNumber[10];
	WORD wControllerType;
	WORD wBufferSize;
	WORD wNumberOfECCBytes;
	WORD vmFirmwareRevision[4];

	// 하드 디스크의 모델 번호
	WORD vwModelNumber[20];
	WORD vwReserved2[13];

	// 디스크의 총 섹터수
	DWORD dwTotalSectors;
	WORD vwReserved3[196];
} HDDINFORMATION;

#pragma pack(pop)

// 하드 디스크를 관리하는 구조체
typedef struct kHDDManagerStruct{
	// HDD 존재 여부와 쓰기를 수행할 수 있는지 여부
	BOOL bHDDDetected;
	BOOL bCanWrite;

	// 인터럽트 발생 여부와 동기화 객체
	volatile BOOL bPrimaryInterruptOccur;
	volatile BOOL bSecondaryInterruptOccur;
	MUTEX stMutex;

	// HDD 정보
	HDDINFORMATION stHDDInformation;
} HDDMANAGER;

BOOL kInitializeHDD(void);
static BYTE kReadHDDStatus(BOOL bPrimary);
static BOOL kWaitForHDDNoBusy(BOOL bPrimary);
static BOOL kWaitForHDDReady(BOOL bPrimary);
void kSetHDDInterruptFlag(BOOL bPrimary, BOOL bFlag);
static BOOL kWaitForHDDInterrupt(BOOL bPrimary);
BOOL kReadHDDInformation(BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation);
static void kSwapByteInWord(WORD* pwData, int iWordCount);
int kReadHDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);
int kWriteHDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);


#endif