#include "MultiProcessor.h"
#include "MPConfigurationTable.h"
#include "AssemblyUtility.h"
#include "LocalAPIC.h"
#include "PIT.h"

// 활성화된 Application Processor 개수
volatile int g_iWakeUpApplicationProcessorCount = 0;
// APIC ID 레지스터의 어드레스
volatile QWORD g_qwAPICIDAddress = 0;

// 로컬 APIC를 활성화하고 AP(Application Processor)를 활성화
BOOL kStartUpApplicationProcessor(void){
	// MP 설정 테이블 분석
	if(kAnalysisMPConfigurationTable() == FALSE)
		return FALSE;

	// 모든 프로세서에서 로컬 APIC를 사용하도록 활성화
	kEnableGlobalLocalAPIC();

	// BSP(Bootstrap Processor)의 로컬 APIC 활성화
	kEnableSoftwareLocalAPIC();

	// AP 를 깨움
	if(kWakeUpApplicationProcessor() == FALSE)
		return FALSE;
}


// AP를 활성화
static BOOL kWakeUpApplicationProcessor(void){
	MPCONFIGURATIONMANAGER* pstMPManager;
	MPCONFIGURATIONTABLEHEADER* pstMPHeader;
	QWORD qwLocalAPICBaseAddress;
	BOOL bInterruptFlag;
	int i;

	bInterruptFlag = kSetInterruptFlag(FALSE);

	// Local APIC memory map address를 MPConfig table 에서 받아옴
	pstMPManager = kGetMPConfigurationManager();
	pstMPHeader = pstMPManager->pstMPConfigurationTableHeader;
	qwLocalAPICBaseAddress = pstMPHeader->dwMemoryMapIOAddressOfLocalAPIC;

	// APIC ID 레지스터의 어드레스(0xfee00020) 저장하여, application processor 가
	// 자신의 apic id 읽을수 있게함
	g_qwAPICIDAddress = qwLocalAPICBaseAddress + APIC_REGISTER_APICID;

	// 하위 인터럽트 커맨드 레지스터(Lower interrupt command register, 0xfee00300)에 초기화(INT)
	// IPI와 시작(start up) IPI 를 전송하여 ap를 깨움
	// IPI == Inter Processor Interrupt

	// 초기화 init ipi 전송
	// 하위 인터럽트 커맨드 레지스터(0xfee00300) 을 사용해서 bsp를 제외한 모든 코어에 init ipi 전송
	// AP는 모호모드 커널 에서 시작
	// All Excluding Self, Edge Trigeer, Assert, Physical Destination, INIT
	*(DWORD*)(qwLocalAPICBaseAddress + APIC_REGISTER_ICR_LOWER) = 
		APIC_DESTINATIONSHORTHAND_ALLEXCLUDINGSELF | APIC_TRIGGERMODE_EDGE | APIC_LEVEL_ASSERT | APIC_DESTINATIONMODE_PHYSICAL | APIC_DELIVERYMODE_INIT;

	// PIT 직접 제어하여 10ms 동안대기
	kWaitUsingDirectPIT(MSTOCOUNT(10));

	// 하위 인터럽트 커맨드 레지스터에서 전달 상태 비트(12th) 를 확인하여 성공여부 확인
	if( *(DWORD*)(qwLocalAPICBaseAddress + APIC_REGISTER_ICR_LOWER) & APIC_DELIVERYSTATUS_PENDING){
		// 타이머 인터럽트가 1초에 100번 발생하도록 재설정
		kInitializePIT(MSTOCOUNT(1), TRUE);

		// 인터럽트 플래그를 복원
		kSetInterruptFlag(bInterruptFlag);
		return FALSE;
	}

	// 시작(start up) IPI전송 2회 전송
	for(i=0; i<2; i++){
		// 하위 인터럽트 커맨드 레지스터을 사용해서 bsp를 제외한
		// 나머지 코어에 start up ipi 전송
		// 보호 모드 커널이 시작하는 0x10000에서 실행시키려고 0x10(0x10000 / 4kb)를
		// 인터럽트 벡터로 설정
		// All Excluding Self, Edge Trigger, Assert, Physical Destination, Start up
		*(DWORD*)(qwLocalAPICBaseAddress + APIC_REGISTER_ICR_LOWER) = 
			APIC_DESTINATIONSHORTHAND_ALLEXCLUDINGSELF | APIC_TRIGGERMODE_EDGE | APIC_LEVEL_ASSERT | APIC_DESTINATIONMODE_PHYSICAL | APIC_DELIVERYMODE_STARTUP | 0x10;

		// PIT 직접 제어 10ms 대기
		kWaitUsingDirectPIT(MSTOCOUNT(10));

		// 하위 인터럽트 커맨드 레지스터(0xfee00300)에서 전달 상태 비트(12) 확인하여 성공여부 확인
		if( *(DWORD*)(qwLocalAPICBaseAddress + APIC_REGISTER_ICR_LOWER) & APIC_DELIVERYSTATUS_PENDING){
			// 타이머 인터럽트가 1초에 100번 발생하도록 재설정
			kInitializePIT(MSTOCOUNT(1), TRUE);

			// 인터럽트 플래그를 복원
			kSetInterruptFlag(bInterruptFlag);
			return FALSE;
		}
	}

	// 타이머 인터럽트 1초에 1000번 발생하도록 재설정
	kInitializePIT(MSTOCOUNT(1), TRUE);

	// 인터럽트 플래그를 복원
	kSetInterruptFlag(bInterruptFlag);

	// Application Processr 가 모두 깨어날때까지 대기
	while(g_iWakeUpApplicationProcessorCount < (pstMPManager->iProcessorCount-1)){
		kSleep(50);
	}

	return TRUE;
}

// APIC ID 레지스터에서 APIC ID 반환
BYTE kGetAPICID(void){
	MPCONFIGURATIONTABLEHEADER* pstMPHeader;
	QWORD qwLocalAPICBaseAddress;

	// APIC ID 어드레스의 값이 설정 되어 있지 않으면 MP 설정 테이블 값에서 값을 읽어서 설정
	if(g_qwAPICIDAddress == 0){
		pstMPHeader = kGetMPConfigurationManager()->pstMPConfigurationTableHeader;
		if(pstMPHeader == NULL)
			return 0;

		// APIC ID 레지스터의 어드레스(0xfee00020) 저장하여 자신의 APIC ID 읽을수 있게함
		qwLocalAPICBaseAddress = pstMPHeader->dwMemoryMapIOAddressOfLocalAPIC;
		g_qwAPICIDAddress = qwLocalAPICBaseAddress + APIC_REGISTER_APICID;
	}

	// APIC ID 레지스터의 Bit 24-31 값을 반환
	return *((DWORD*)g_qwAPICIDAddress) >> 24;
}