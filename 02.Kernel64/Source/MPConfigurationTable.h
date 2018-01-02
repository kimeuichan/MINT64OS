#ifndef __MPCONFIGURATIONTLABLE_H__

#define __MPCONFIGURATIONTLABLE_H__

#include "Types.h"

/***** 매크로 정의 *****/
// 시그너처
#define MP_FLOATINGPOINTER_SIGNATURE "_MP_" // MP 플로팅 포인터 시그너처

// MP 플로팅 포인터 검색
#define MP_SEARCH1_EBDA_ADDRESS         (*(WORD*)0x040E) * 16   // 확장 BIOS 데이터 영역 어드레스(세그먼트 시작 에드레스가 들어 있으므로 실제 물리 어드레스는 16을 곱함)
#define MP_SEARCH2_SYSEMBASEMEMORY      (*(WORD*)0x0413) * 1024 // 시스템 기본 메모리 크기(KByte 단위로 들어 있으므로 실제 메모리 크기는 1024을 곱합)
#define MP_SEARCH3_BIOSROM_STARTADDRESS 0x0F0000                // BIOS 롬 영역 시작 어드레스
#define MP_SEARCH3_BIOSROM_ENDADDRESS   0x0FFFFF                // BIOS 롬 영역 끝 어드레스

// MP 플로팅 포인터 - MP 특성 바이트1~5(1바이트*5)
#define MP_FLOATINGPOINTER_FEATUREBYTE1_USEMPTABLE 0x00 // MP 설정 테이블 사용 (0:MP 설정 테이블 사용, !0:MultiProcessor Specification 문서에서 정의한 기본 설정 사용)
#define MP_FLOATINGPOINTER_FEATUREBYTE2_PICMODE    0x80 // PIC 모드 지원(비트 7=1:PIC 모드 지원, 비트 7=0:가상 연결 모드 지원, 비트 0~6:예약됨)

// 기본 MP 설정 테이블 엔트리 - 엔트리 타입(1바이트)
#define MP_ENTRYTYPE_PROCESSOR                0 // 프로세서 엔트리
#define MP_ENTRYTYPE_BUS                      1 // 버스 엔트리
#define MP_ENTRYTYPE_IOAPIC                   2 // IO APIC 엔트리
#define MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT    3 // IO 인터럽트 지정 엔트리
#define MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT 4 // 로컬 인터럽트 지정 엔트리

// 프로세서 엔트리 - CPU 플래그(1바이트)
#define MP_PROCESSOR_CPUFLAGS_ENABLE 0x01 // 프로세서/코어 사용 가능(비트 0=1:프로세서/코어 사용 가능, 비트 0=0:프로세서/코어 사용 불가능)
#define MP_PROCESSOR_CPUFLAGS_BSP    0x02 // BSP(비트 1=1:BSP, 비트 1=0:AP) [참고]BSP:Bootstrap Processor, AP:Application Processor

// 버스 엔트리 - 버스 타입 문자열(6바이트)
#define MP_BUS_TYPESTRING_ISA          "ISA"    // Industry Standard Architecture
#define MP_BUS_TYPESTRING_PCI          "PCI"    // Peripheral Component Interconnect
#define MP_BUS_TYPESTRING_PCMCIA       "PCMCIA" // PC Memory Card International Association
#define MP_BUS_TYPESTRING_VESALOCALBUS "VL"     // VESA Local Bus

// IO APIC 엔트리 - IO APIC 플래그(1바이트)
#define MP_IOAPIC_FLAGS_ENABLE 0x01 // IO APIC 사용 가능(비트 0=1:IO APIC 사용 가능, 비트 0=0:IO APIC 사용 불가능)

// IO 인터럽트 지정 엔트리, 로컬 인터럽트 지정 엔트리 - 인터럽트 타입(1바이트)
#define MP_INTERRUPT_TYPE_INT    0 // 인터럽트 발생시 IO APIC에 지정된 인터럽트 벡터로 로컬 APIC에 전달하는 벡터 인터럽트
#define MP_INTERRUPT_TYPE_NMI    1 // 마스크 할 수 없는 인터럽트(Non-Maskable Interrupt)
#define MP_INTERRUPT_TYPE_SMI    2 // 시스템 관리 인터럽트(System Management Interrupt)
#define MP_INTERRUPT_TYPE_EXTINT 3 // PIC에서 전달되는 인터럽트

// IO 인터럽트 지정 엔트리, 로컬 인터럽트 지정 엔트리 - 인터럽트 플래그(2바이트)
/**
 * @ PO(Polarity, 2비트) : 인터럽트 신호의 극성
 *   - 비트 1=0 비트 0=0 : 버스 타입에 따라서 설정
 *   - 비트 1=0 비트 0=1 : 1일때 활성화
 *   - 비트 1=1 비트 0=0 : 예약됨
 *   - 비트 1=1 비트 0=1 : 0일대 활성화
 *
 * @ EL(Edge-Level Trigger, 2비트) : 인터럽트 신호의 트리거 모드
 *   - 비트 3=0 비트 2=0 : 버스 타입에 따라서 설정
 *   - 비트 3=0 비트 2=1 : 엣지 트리거
 *   - 비트 3=1 비트 2=0 : 예약됨
 *   - 비트 3=1 비트 2=1 : 레벨 트리거
 */
#define MP_INTERRUPT_FLAGS_CONFORMPOLARITY 0x00 // 극성을 버스 타입에 따라서 설정
#define MP_INTERRUPT_FLAGS_ACTIVEHIGH      0x01 // 1일때 활성화
#define MP_INTERRUPT_FLAGS_ACTIVELOW       0x03 // 0일때 활성화
#define MP_INTERRUPT_FLAGS_CONFORMTRIGGER  0x00 // 트리거 모드를 버스 타입에 따라서 설정
#define MP_INTERRUPT_FLAGS_EDGETRIGGERED   0x04 // 엣지 트리거
#define MP_INTERRUPT_FLAGS_LEVELTRIGGERED  0x0C // 레벨 트리거

#pragma pack(push, 1)

// MP 플로팅 포인터 구조체(16byte)
typedef struct kMPFloatingPointerStruct{
	// MP 플로팅 포인터 시그니쳐
	char vcSignature[4];
	// MP 테이블 시작 어드레스
	DWORD dwMPConfigurationTableAddress;
	// MP 플로팅 포인터 길이
	BYTE bLength;
	// MultiProcessor Specification 버전
	BYTE bRevision;
	// 체크섬
	BYTE bChecksum;
	// MP 특성 바이트[5]
	BYTE vbMPFeatureByte[5];
} MPFLOATINGPOINTER;

// MP 설정 테이블 구조체(44byte)
typedef struct kMPConfigurationTableHeaderStruct{
	// 시그니쳐
	char vcSignature[4];
	// 기본 테이블 길이
	WORD wBaseTableLength;
	// MultiProcessor Specification 버전
	BYTE bRevision;
	// 체크섬
	BYTE bChecksum;
	// 하드웨어를 만든 OEM ID (ASCII)
	char vcOEMIDString[8];
	// Product ID
	char vcProductIDString[12];
	// OEM 테이블 포인터
	DWORD dwOEMTablePointerAddress;
	// OEM 테이블 사이즈
	WORD wOEMTableSize;
	// MP 설정 테이블 엔트리 갯수
	WORD wEntryCount;
	// 로컬 APIC의 메모리맵 IO 어드레스
	DWORD dwMemoryMapIOAddressOfLocalAPIC;
	// 확장 테이블 길이
	WORD wExtendedTableLength;
	// 확장 테이블 체크섬
	BYTE bExtendedTableChecksum;
	// 예약됨
	BYTE bReserved;
} MPCONFIGURATIONTABLEHEADER;

// MP 테이블 PROCESSOR 구조체(20byte)
typedef struct kProcessorEntryStruct{
	// 엔트리 타입(0)
	BYTE bEntryType;
	// 로컬 APIC ID
	BYTE bLocalAPICID;
	// 로컬 APIC 버전
	BYTE bLocalAPICVersion;
	// CPU 플래그
	BYTE bCPUFlags;
	// CPU 시그니쳐
	BYTE vbCPUSignature[4];
	// 특성 플래그
	DWORD dwFeatureFlags;
	// 예약된 영역
	DWORD vdwReserved[2];
} PROCESSORENTRY;

// MP 테이블 BUS 구조체(8byte)
typedef struct kBusEntryStruct{
	// 엔트리 타입(1)
	BYTE bEntryType;
	// 버스 ID
	BYTE bBusID;
	// 버스 타입 문자열
	char vcBusTypeString[6];
} BUSENTRY;

// MP 테이블 IO APIC 구조체(8byte)
typedef struct kIOAPICEntryStruct{
	// 엔트리 타입(2)
	BYTE bEntryType;
	// IO APIC ID
	BYTE bIOAPICID;
	// IO APIC 버전
	BYTE bIOAPICVersion;
	// IO APIC 플래그
	BYTE bIOAPICFlags;
	// IO APIC 의 메모리 맵 IO 어드레스
	DWORD dwMemoryMapIOAddress;
} IOAPICENTRY;

// MP 테이블 IO 인터럽트 구조체(8byte)
typedef struct kIOInterruptAssignmentEntryStruct{
	// 엔트리 타입(3)
	BYTE bEntryType;
	// 인터럽트 타입
	BYTE bInterruptType;
	// 인터럽트 플래그
	WORD wInterruptFlags;
	// 인터럽트 발생한 버스 ID
	BYTE bSourceBUSID;
	// 인터럽트 발생한 버스 IRQ
	BYTE bSourceBUSIRQ;
	// 전달할 IO APIC ID
	BYTE bDestinationIOAPICID;
	// 전달할 IO APIC INTIN
	BYTE bDestinationIOAPICINTIN;
} IOINTERRUPTASSIGNMENTENTRY;

// MP 테이블 로컬 인터럽트 구조체(8byte)
typedef struct kLocalInterruptAssignmentEntryStruct{
	// 엔트리 타입(4)
	BYTE bEntryType;
	// 인터럽트 타입
	BYTE bInterruptType;
	// 인터럽트 플래그
	WORD wInterruptFlags;
	// 인터럽트 발생한 버스 ID
	BYTE bSourceBUSID;
	// 인터럽트 발생한 버스 IRQ
	BYTE bSourceBUSIRQ;
	// 전달할 APIC ID
	BYTE bDestinationLocalAPICID;
	// 전달할 APIC LINTIN
	BYTE bDestinationLocalAPICLINTIN;
} LOCALINTERRUPTASSIGNMENTENTRY;

// MP 설정 테이블 매니저
typedef struct kMPConfigurationManagerStruct {
	// MP 플로팅 포인터
	MPFLOATINGPOINTER* pstMPFloatingPointer;
	// MP 설정 테이블 헤더
	MPCONFIGURATIONTABLEHEADER* pstMPConfigurationTableHeader; 
	// 기본 MP 설정 테이블 엔트리 시작 에드레스
	QWORD qwBaseEntryStartAddress;
	// 프로세서/코어 개수                             
	int iProcessorCount;
	// PIC 모드 지원 여부                                       
	BOOL bUsePICMode;
	// ISA 버스 ID                                          
	BYTE bISABusID;                                            
} MPCONFIGURATIONMANAGER;

#pragma pack(pop)

BOOL kFindMPFloatingPointerAddress(QWORD* pdwAddress);
BOOL kAnalysisMPConfigurationTable(void);
MPCONFIGURATIONMANAGER* kGetMPConfigurationManager(void);
void kPrintMPConfigurationTable(void);
IOAPICENTRY* kFindIOAPICEntryForISA( void );
int kGetProcessorCount(void);

#endif