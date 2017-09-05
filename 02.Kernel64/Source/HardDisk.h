#ifndef __HARD_DISK_H__
#define __HARD_DISK_H__

#include "Types.h"
#include "Synchronization.h"

/***** 매크로 정의 *****/
// 하드 디스크 컨트롤러의 I/O 포트 기준값
#define HDD_PORT_PRIMARYBASE   0x1F0 // 첫번째 PATA 포트 기준값
#define HDD_PORT_SECONDARYBASE 0x170 // 두번째 PATA 포트 기준값

// 하드 디스크 컨트롤러의 I/O 포트 인덱스
#define HDD_PORT_INDEX_DATA          0x00  // 데이터 레지스터(0x1F0, 0x170): 읽기/쓰기, 2 byte 크기, 하드 디스크로의 송/수신 데이터를 저장
#define HDD_PORT_INDEX_SECTORCOUNT   0x02  // 섹터 수 레지스터(0x1F2, 0x172): 읽기/쓰기, 1 byte 크기, 섹터 수를 저장(1~256섹터까지 가능, 0을 입력하면 256을 의미)
#define HDD_PORT_INDEX_SECTORNUMBER  0x03  // 섹터 번호 레지스터(0x1F3, 0x173): 읽기/쓰기, 1 byte 크기, 섹터 번호를 저장
#define HDD_PORT_INDEX_CYLINDERLSB   0x04  // 실리더 LSB 레지스터(0x1F4, 0x174): 읽기/쓰기, 1 byte 크기, 실린더 번호의 하위 8비트를 저장
#define HDD_PORT_INDEX_CYLINDERMSB   0x05  // 실리더 MSB 레지스터(0x1F5, 0x175): 읽기/쓰기, 1 byte 크기, 실린더 번호의 상위 8비트를 저장
#define HDD_PORT_INDEX_DRIVEANDHEAD  0x06  // 드라이버/헤드 레지스터(0x1F6, 0x176): 읽기/쓰기, 1 byte 크기, 드라이브 번호와 헤드 번호를 저장
#define HDD_PORT_INDEX_STATUS        0x07  // 상태 레지스터(0x1F7, 0x177): 읽기, 1 byte 크기, 하드 디스크의 상태를 저장
#define HDD_PORT_INDEX_COMMAND       0x07  // 커맨드 레지스터(0x1F7, 0x177): 쓰기, 1 byte 크기, 하드 디스크로 송신할 커맨드를 저장
#define HDD_PORT_INDEX_DIGITALOUTPUT 0x206 // 디지털 출력 레지스터(0x3F6, 0x376): 읽기/쓰기, 1 byte 크기, 인터럽트 활성화와 하드 디스크 리셋을 담당

// 커맨드 레지스터(8비트)의 커맨드
#define HDD_COMMAND_READ     0x20 // 섹터 읽기 : 필요한 레지스터->섹터 수 레지스터, 섹터 번호 레지스터, 실리더 LSB/MSB 레지스터, 드라이버/헤드 레지스터
#define HDD_COMMAND_WRITE    0x30 // 섹터 쓰기 : 필요한 레지스터->섹터 수 레지스터, 섹터 번호 레지스터, 실리더 LSB/MSB 레지스터, 드라이버/헤드 레지스터
#define HDD_COMMAND_IDENTIFY 0xEC // 드라이브 인식(하드 디스크 정보 읽기): 필요한 레지스터->드라이버/헤드 레지스터

// 상태 레지스터(8비트)의 필드
#define HDD_STATUS_ERROR         0x01 // ERR(비트 0): Error, 이전에 수행했던 커맨드에 에러가 발생했음을 의미
#define HDD_STATUS_INDEX         0x02 // IDX(비트 1): Index, 디스크의 인덱스 마크가 검출되었음을 의미
#define HDD_STATUS_CORRECTEDDATA 0x04 // CORR(비트 2): Correctable Data Error, 작업 도중 데이터 에러가 발생했으나 ECC 정보로 복구했음을 의미
#define HDD_STATUS_DATAREQUEST   0x08 // DRQ(비트 3): Data Request, 하드 디스크가 데이터를 송/수신 가능한 상태를 의미
#define HDD_STATUS_SEEKCOMPLETE  0x10 // DSC(비트 4): Device Seek Complete, 악세스 암의 헤드가 원하는 위치로 옮겨졌음을 의미
#define HDD_STATUS_WRITEFAULT    0x20 // DF(비트 5): Device Fault, 작업 도중 문제가 발생했음을 의미
#define HDD_STATUS_READY         0x40 // DRDY(비트 6): Device Ready, 하드 디스크가 커맨드를 수신 가능한 상태를 의미
#define HDD_STATUS_BUSY          0x80 // BSY(비트 7): Busy, 하드 디스크가 커맨드를 실행중인 상태를 의미

// 드라이버/헤드 레지스터(8비트)의 필드
/* [참고]
 * LBA 모드(비트 6)=0 : CHS 모드->섹터 수 레지스터에는 섹터 수를, 섹터 번호 레지스터에는 섹터 번호를, 실리더 LSB/MSB 레지스터에는 실린더 번호를, 드라이버/헤드 레지스터의 헤드 번호 필드에는 헤드 번호를 저장하는 방식
 * LBA 모드(비트 6)=1 : LBA 모드->섹터 수 레지스터에는 섹터 수를 저장하고 , 나머지 레지스터는 LBA 어드레스로 통합되어
 *                           LBA 어드레스(28비트)의 [비트 0~7:섹터 번호 레지스터], [비트 8~15:실리더 LSB 레지스터], [비트 16~23:실리더 MSB 레지스터], [비트 24~27:드라이버/헤드 레지스터의 헤드 번호 필드]를 저장하는 방식
 * 드라이브 번호(비트 4)=0 : 마스터 하드 디스크와 송/수신
 * 드라이브 번호(비트 4)=1 : 슬레이브 하드 디스크와 송/수신
 */
#define HDD_DRIVEANDHEAD_LBA   0xE0 // 1110 0000 : 고정값(비트 7)=1, LBA 모드(비트 6)=1, 고정값(비트 5)=1, 드라이브 번호(비트 4)=0, 헤드 번호(비트 3~0)=0000
#define HDD_DRIVEANDHEAD_SLAVE 0x10 // 0001 0000 : 드라이브 번호(비트 4)=1

// 하드 디스크의 응답을 대기하는 시간(ms)
#define HDD_WAITTIME 500

// 하드 디스크에 한번에 읽거나 쓸 수 있는 최대 섹터 수
#define HDD_MAXBULKSECTORCOUNT 256

/***** 구조체 정의 *****/
#pragma pack(push, 1)

typedef struct kHDDInformationStruct{
	// 설정값
	WORD wConfiguration;

	// 실린더 수(CHS 모드에서 사용)
	WORD wNumberOfCylinder;
	WORD wReserved1;

	// 헤드 수(CHS 모드에서 사용)
	WORD wNumberOfHead;
	WORD wUnformattedBytesPerTrack;
	WORD wUnformattedBytesPerSector;

	// 실린더당 섹터 수(CHS 모드에서 사용)
	WORD wNumberOfSectorPerCylinder;
	WORD wInterSectorGap;
	WORD wBytesInPhaseLock;
	WORD wNumberOfVendorUniqueStatusWord;

	// 시리얼 번호
	WORD vwSerialNumber[10];
	WORD wControllerType;
	WORD wBufferSize;
	WORD wNumberOfECCBytes;
	WORD vwFirmwareRevision[4];

	// 모델 번호
	WORD vwModelNumber[20];
	WORD vwReserved2[13];

	// 총 섹터 수(LBA 모드에서 사용)
	DWORD dwTotalSectors;
	WORD vwReserved3[196];
} HDDINFORMATION;

typedef struct kHDDManagerStruct{
	BOOL bHDDDetected;                      // 하드 디스크 존재 여부
	BOOL bCanWrite;                         // 쓰기 가능 여부(현재 QEMU로 실행했을 때만 하드 디스크에 쓰기 가능)
	volatile BOOL bPrimaryInterruptOccur;   // 첫번째 인터럽트 플래그(인터럽트 발생 여부)
	volatile BOOL bSecondaryInterruptOccur; // 두번째 인터럽트 플래그(인터럽트 발생 여부)
	MUTEX stMutex;                          // 뮤텍스 동기화 객체
	HDDINFORMATION stHDDInformation;        // 하드 디스크 정보
} HDDMANAGER;

#pragma pack(pop)

/***** 함수 정의 *****/
BOOL kInitializeHDD(void);
BOOL kReadHDDInformation(BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation);
int kReadHDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);
int kWriteHDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);
void kSetHDDInterruptFlag(BOOL bPrimary, BOOL bFlag);
static void kSwapByteInWord(WORD* pwData, int iWordCount);
static BYTE kReadHDDStatus(BOOL bPrimary);
static BOOL kIsHDDBusy(BOOL bPrimary);  // [주의]이 함수는 책에서 구현되어 있지 않아서, 내가 직접 구현했음
static BOOL kIsHDDReady(BOOL bPrimary); // [주의]이 함수는 책에서 구현되어 있지 않아서, 내가 직접 구현했음
static BOOL kWaitForHDDNoBusy(BOOL bPrimary);
static BOOL kWaitForHDDReady(BOOL bPrimary);
static BOOL kWaitForHDDInterrupt(BOOL bPrimary);

#endif // __HARD_DISK_H__
