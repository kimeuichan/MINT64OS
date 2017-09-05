#ifndef __HARD_DISK_H__
#define __HARD_DISK_H__

#include "Types.h"
#include "Synchronization.h"

/***** ��ũ�� ���� *****/
// �ϵ� ��ũ ��Ʈ�ѷ��� I/O ��Ʈ ���ذ�
#define HDD_PORT_PRIMARYBASE   0x1F0 // ù��° PATA ��Ʈ ���ذ�
#define HDD_PORT_SECONDARYBASE 0x170 // �ι�° PATA ��Ʈ ���ذ�

// �ϵ� ��ũ ��Ʈ�ѷ��� I/O ��Ʈ �ε���
#define HDD_PORT_INDEX_DATA          0x00  // ������ ��������(0x1F0, 0x170): �б�/����, 2 byte ũ��, �ϵ� ��ũ���� ��/���� �����͸� ����
#define HDD_PORT_INDEX_SECTORCOUNT   0x02  // ���� �� ��������(0x1F2, 0x172): �б�/����, 1 byte ũ��, ���� ���� ����(1~256���ͱ��� ����, 0�� �Է��ϸ� 256�� �ǹ�)
#define HDD_PORT_INDEX_SECTORNUMBER  0x03  // ���� ��ȣ ��������(0x1F3, 0x173): �б�/����, 1 byte ũ��, ���� ��ȣ�� ����
#define HDD_PORT_INDEX_CYLINDERLSB   0x04  // �Ǹ��� LSB ��������(0x1F4, 0x174): �б�/����, 1 byte ũ��, �Ǹ��� ��ȣ�� ���� 8��Ʈ�� ����
#define HDD_PORT_INDEX_CYLINDERMSB   0x05  // �Ǹ��� MSB ��������(0x1F5, 0x175): �б�/����, 1 byte ũ��, �Ǹ��� ��ȣ�� ���� 8��Ʈ�� ����
#define HDD_PORT_INDEX_DRIVEANDHEAD  0x06  // ����̹�/��� ��������(0x1F6, 0x176): �б�/����, 1 byte ũ��, ����̺� ��ȣ�� ��� ��ȣ�� ����
#define HDD_PORT_INDEX_STATUS        0x07  // ���� ��������(0x1F7, 0x177): �б�, 1 byte ũ��, �ϵ� ��ũ�� ���¸� ����
#define HDD_PORT_INDEX_COMMAND       0x07  // Ŀ�ǵ� ��������(0x1F7, 0x177): ����, 1 byte ũ��, �ϵ� ��ũ�� �۽��� Ŀ�ǵ带 ����
#define HDD_PORT_INDEX_DIGITALOUTPUT 0x206 // ������ ��� ��������(0x3F6, 0x376): �б�/����, 1 byte ũ��, ���ͷ�Ʈ Ȱ��ȭ�� �ϵ� ��ũ ������ ���

// Ŀ�ǵ� ��������(8��Ʈ)�� Ŀ�ǵ�
#define HDD_COMMAND_READ     0x20 // ���� �б� : �ʿ��� ��������->���� �� ��������, ���� ��ȣ ��������, �Ǹ��� LSB/MSB ��������, ����̹�/��� ��������
#define HDD_COMMAND_WRITE    0x30 // ���� ���� : �ʿ��� ��������->���� �� ��������, ���� ��ȣ ��������, �Ǹ��� LSB/MSB ��������, ����̹�/��� ��������
#define HDD_COMMAND_IDENTIFY 0xEC // ����̺� �ν�(�ϵ� ��ũ ���� �б�): �ʿ��� ��������->����̹�/��� ��������

// ���� ��������(8��Ʈ)�� �ʵ�
#define HDD_STATUS_ERROR         0x01 // ERR(��Ʈ 0): Error, ������ �����ߴ� Ŀ�ǵ忡 ������ �߻������� �ǹ�
#define HDD_STATUS_INDEX         0x02 // IDX(��Ʈ 1): Index, ��ũ�� �ε��� ��ũ�� ����Ǿ����� �ǹ�
#define HDD_STATUS_CORRECTEDDATA 0x04 // CORR(��Ʈ 2): Correctable Data Error, �۾� ���� ������ ������ �߻������� ECC ������ ���������� �ǹ�
#define HDD_STATUS_DATAREQUEST   0x08 // DRQ(��Ʈ 3): Data Request, �ϵ� ��ũ�� �����͸� ��/���� ������ ���¸� �ǹ�
#define HDD_STATUS_SEEKCOMPLETE  0x10 // DSC(��Ʈ 4): Device Seek Complete, �Ǽ��� ���� ��尡 ���ϴ� ��ġ�� �Ű������� �ǹ�
#define HDD_STATUS_WRITEFAULT    0x20 // DF(��Ʈ 5): Device Fault, �۾� ���� ������ �߻������� �ǹ�
#define HDD_STATUS_READY         0x40 // DRDY(��Ʈ 6): Device Ready, �ϵ� ��ũ�� Ŀ�ǵ带 ���� ������ ���¸� �ǹ�
#define HDD_STATUS_BUSY          0x80 // BSY(��Ʈ 7): Busy, �ϵ� ��ũ�� Ŀ�ǵ带 �������� ���¸� �ǹ�

// ����̹�/��� ��������(8��Ʈ)�� �ʵ�
/* [����]
 * LBA ���(��Ʈ 6)=0 : CHS ���->���� �� �������Ϳ��� ���� ����, ���� ��ȣ �������Ϳ��� ���� ��ȣ��, �Ǹ��� LSB/MSB �������Ϳ��� �Ǹ��� ��ȣ��, ����̹�/��� ���������� ��� ��ȣ �ʵ忡�� ��� ��ȣ�� �����ϴ� ���
 * LBA ���(��Ʈ 6)=1 : LBA ���->���� �� �������Ϳ��� ���� ���� �����ϰ� , ������ �������ʹ� LBA ��巹���� ���յǾ�
 *                           LBA ��巹��(28��Ʈ)�� [��Ʈ 0~7:���� ��ȣ ��������], [��Ʈ 8~15:�Ǹ��� LSB ��������], [��Ʈ 16~23:�Ǹ��� MSB ��������], [��Ʈ 24~27:����̹�/��� ���������� ��� ��ȣ �ʵ�]�� �����ϴ� ���
 * ����̺� ��ȣ(��Ʈ 4)=0 : ������ �ϵ� ��ũ�� ��/����
 * ����̺� ��ȣ(��Ʈ 4)=1 : �����̺� �ϵ� ��ũ�� ��/����
 */
#define HDD_DRIVEANDHEAD_LBA   0xE0 // 1110 0000 : ������(��Ʈ 7)=1, LBA ���(��Ʈ 6)=1, ������(��Ʈ 5)=1, ����̺� ��ȣ(��Ʈ 4)=0, ��� ��ȣ(��Ʈ 3~0)=0000
#define HDD_DRIVEANDHEAD_SLAVE 0x10 // 0001 0000 : ����̺� ��ȣ(��Ʈ 4)=1

// �ϵ� ��ũ�� ������ ����ϴ� �ð�(ms)
#define HDD_WAITTIME 500

// �ϵ� ��ũ�� �ѹ��� �аų� �� �� �ִ� �ִ� ���� ��
#define HDD_MAXBULKSECTORCOUNT 256

/***** ����ü ���� *****/
#pragma pack(push, 1)

typedef struct kHDDInformationStruct{
	// ������
	WORD wConfiguration;

	// �Ǹ��� ��(CHS ��忡�� ���)
	WORD wNumberOfCylinder;
	WORD wReserved1;

	// ��� ��(CHS ��忡�� ���)
	WORD wNumberOfHead;
	WORD wUnformattedBytesPerTrack;
	WORD wUnformattedBytesPerSector;

	// �Ǹ����� ���� ��(CHS ��忡�� ���)
	WORD wNumberOfSectorPerCylinder;
	WORD wInterSectorGap;
	WORD wBytesInPhaseLock;
	WORD wNumberOfVendorUniqueStatusWord;

	// �ø��� ��ȣ
	WORD vwSerialNumber[10];
	WORD wControllerType;
	WORD wBufferSize;
	WORD wNumberOfECCBytes;
	WORD vwFirmwareRevision[4];

	// �� ��ȣ
	WORD vwModelNumber[20];
	WORD vwReserved2[13];

	// �� ���� ��(LBA ��忡�� ���)
	DWORD dwTotalSectors;
	WORD vwReserved3[196];
} HDDINFORMATION;

typedef struct kHDDManagerStruct{
	BOOL bHDDDetected;                      // �ϵ� ��ũ ���� ����
	BOOL bCanWrite;                         // ���� ���� ����(���� QEMU�� �������� ���� �ϵ� ��ũ�� ���� ����)
	volatile BOOL bPrimaryInterruptOccur;   // ù��° ���ͷ�Ʈ �÷���(���ͷ�Ʈ �߻� ����)
	volatile BOOL bSecondaryInterruptOccur; // �ι�° ���ͷ�Ʈ �÷���(���ͷ�Ʈ �߻� ����)
	MUTEX stMutex;                          // ���ؽ� ����ȭ ��ü
	HDDINFORMATION stHDDInformation;        // �ϵ� ��ũ ����
} HDDMANAGER;

#pragma pack(pop)

/***** �Լ� ���� *****/
BOOL kInitializeHDD(void);
BOOL kReadHDDInformation(BOOL bPrimary, BOOL bMaster, HDDINFORMATION* pstHDDInformation);
int kReadHDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);
int kWriteHDDSector(BOOL bPrimary, BOOL bMaster, DWORD dwLBA, int iSectorCount, char* pcBuffer);
void kSetHDDInterruptFlag(BOOL bPrimary, BOOL bFlag);
static void kSwapByteInWord(WORD* pwData, int iWordCount);
static BYTE kReadHDDStatus(BOOL bPrimary);
static BOOL kIsHDDBusy(BOOL bPrimary);  // [����]�� �Լ��� å���� �����Ǿ� ���� �ʾƼ�, ���� ���� ��������
static BOOL kIsHDDReady(BOOL bPrimary); // [����]�� �Լ��� å���� �����Ǿ� ���� �ʾƼ�, ���� ���� ��������
static BOOL kWaitForHDDNoBusy(BOOL bPrimary);
static BOOL kWaitForHDDReady(BOOL bPrimary);
static BOOL kWaitForHDDInterrupt(BOOL bPrimary);

#endif // __HARD_DISK_H__
