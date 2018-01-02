#include "IOAPIC.h"
#include "MPConfigurationTable.h"
#include "PIC.h"


// I/O APIC 관리 자료 구조
static IOAPICMANAGER gs_stIOAPICManager;

// ISA 버스가 연결된 I/O APIC 의 기준 어드레스를 반환
QWORD kGetIOAPICBaseAddressOfISA(void){
	MPCONFIGURATIONMANAGER* pstMPManager;
	IOAPICENTRY* pstIOAPICEntry;

	// I/O APIC의 어드레스가 저장되어 있지 않으면 엔트리를 찾아서 저장
	if(gs_stIOAPICManager.qwIOAPICBaseAddressOfISA == NULL){
		pstIOAPICEntry = kFindIOAPICEntryForISA();
		if(pstIOAPICEntry != NULL){
			gs_stIOAPICManager.qwIOAPICBaseAddressOfISA = pstIOAPICEntry->dwMemoryMapIOAddress & 0xffffffff;
		}
	}

	return gs_stIOAPICManager.qwIOAPICBaseAddressOfISA;
}

// I/O 리다이렉션 테이블 자료구조에 값을 설정
void kSetIOAPICRedirectionEntry(IOREDIRECTIONTABLE* pstEntry, BYTE bAPICID, BYTE bInterruptMask, BYTE bFlagsAndDeliveryMode, BYTE bVector){
	kMemSet(pstEntry, 0, sizeof(IOREDIRECTIONTABLE));

	pstEntry->bDestination = bAPICID;
	pstEntry->bFlagsAndDeliveryMode = bFlagsAndDeliveryMode;
	pstEntry->bInterruptMask = bInterruptMask;
	pstEntry->bVector = bVector;
}

// 인터럽트 입력 핀에 해당하는 I/O 리다이렉션 테이블에서 값을 읽음
void kReadIOAPICRedirectionTable(int iINTIN, IOREDIRECTIONTABLE* pstEntry){
	QWORD* pqwData;
	QWORD qwIOAPICBaseAddress;

	// isa 버스가 연결된 I/O Apic 메모리 맵 i/o 어드레스
	qwIOAPICBaseAddress = kGetIOAPICBaseAddressOfISA();

	// i/o 리다이렉션 테이블을 8byte ㅣ음로 8byte 정수로 변환해서 처리
	pqwData = (QWORD*)pstEntry;

	// i/o 리다이렉션 테이블의 상위 4바이트를 읽음
	// i/o 리다이렉션 테이블은 상위 레지스터와 하위 레지스터가 한쌍이므로 2를 곱하여
	// 해당 io 리다이렉션 테이블 레지스터의 인덱스를 계산

	// i/o 레지스터 선택 레지스터에 상위 io 리다이렉션 테이블 레지스터의 인덱스를 전송
	*(DWORD*)(qwIOAPICBaseAddress + IOAPIC_REGISTER_IOREGISTERSELECTOR) = IOAPIC_REGISTERINDEX_HIGHIOREDIRECTIONTABLE + iINTIN*2;
	// i/o 윈도우 레지스터에서 상위 레지스터 값을 읽음
	*pqwData = *(DWORD*)(qwIOAPICBaseAddress + IOAPIC_REGISTER_IOWINDOW);
	*pqwData = *pqwData<<32;

	// i/o 리다이렉션 테이블의 하위 4바이트를 읽음
	// i/o 리다이렉션 테이블은 상위 레지스터와 하위 레지스터가 한쌍이므로 2를 곱하여
	// 해당 io 리다이렉션 테이블 레지스터의 인덱스를 계산
	// i/o 레지스터 선택 레지스터에 상위 io 리다이렉션 테이블 레지스터의 인덱스를 전송
	*(DWORD*)(qwIOAPICBaseAddress + IOAPIC_REGISTER_IOREGISTERSELECTOR) = IOAPIC_REGISTERINDEX_LOWIOREDIRECTIONTABLE + iINTIN*2;
	// i/o 윈도우 레지스터에서 상위 레지스터 값을 읽음
	*pqwData |= *(DWORD*)(qwIOAPICBaseAddress + IOAPIC_REGISTER_IOWINDOW);
}

// 인터럽트 입력 핀에 해당하는 i/o 리다이렉션 테이블에 값을 씀
void kWriteIOAPICRedirectionTable(int iINTIN, IOREDIRECTIONTABLE* pstEntry){
	QWORD *pqwData;
	QWORD qwIOAPICBaseAddress;

	qwIOAPICBaseAddress = kGetIOAPICBaseAddressOfISA();

	pqwData = (QWORD*)pstEntry;

	// 리다이렉션 테이블의 상위 인덱스를 설정한 후 pstEntry 의 상위 32비트 를 씀
	*(DWORD*)(qwIOAPICBaseAddress + IOAPIC_REGISTER_IOREGISTERSELECTOR) = IOAPIC_REGISTERINDEX_HIGHIOREDIRECTIONTABLE + iINTIN*2;
	*(DWORD*)(qwIOAPICBaseAddress + IOAPIC_REGISTER_IOWINDOW) = *pqwData >> 32;

	// 리다이렉션 테이블의 상위 인덱스를 설정한 후 pstEntry 의 하위 32비트 를 씀
	*(DWORD*)(qwIOAPICBaseAddress + IOAPIC_REGISTER_IOREGISTERSELECTOR) = IOAPIC_REGISTERINDEX_LOWIOREDIRECTIONTABLE + iINTIN*2;
	*(DWORD*)(qwIOAPICBaseAddress + IOAPIC_REGISTER_IOWINDOW) = *pqwData;
}

// i/o apic 연결된 모든 인터럽트 핀을 마스크하여 인터럽트가 전달되지 않도록함
void kMaskAllInterruptInIOAPIC(void){
	IOREDIRECTIONTABLE stEntry;
	int i;

	// 모든 인터럽트 비활성화
	for(i=0; i<IOAPIC_MAXIOREDIRECTIONTABLECOUNT; i++){
		// io 리다이렉션 테이블을 읽어 인터럽트 마스크 필드만 1로 설정(0th)
		kReadIOAPICRedirectionTable(i, &stEntry);
		stEntry.bInterruptMask |= IOAPIC_INTERRUPT_MASK;
		kWriteIOAPICRedirectionTable(i, &stEntry);
	}
}

// i/o apic 의 리다이렉션 테이블 초기화
void kInitializeIORedirectionTable(void){
	MPCONFIGURATIONMANAGER* pstMPManager;
	MPCONFIGURATIONTABLEHEADER* pstMPHeader;
	IOINTERRUPTASSIGNMENTENTRY* pstIOAssignmentEntry;
	IOREDIRECTIONTABLE stIORedirectionEntry;
	DWORD qwEntryAddress;
	BYTE bEntryType;
	BYTE bDestination;
	int i;

	// i/o apic 관리하는 자료구조 초기화
	kMemSet( &gs_stIOAPICManager, 0, sizeof(gs_stIOAPICManager));

	//i/o apic 메모리 맵 i/o 어드레스 저장, 아래 함수에서 내부적으로 처리
	kGetIOAPICBaseAddressOfISA();

	// irq를 i/o apic 의 intin 핀과 연결한 테이블 초기화
	for(i=0; i<IOAPIC_MAXIRQTOINTINMAPCOUNT; i++)
		gs_stIOAPICManager.vbIRQToINTINMap[i] = 0xff;

	// i/o apic 마스크하여 인터럽트가 발생하지 않도록 하고 i/o 리다이렉션 테이블 초기화
	kMaskAllInterruptInIOAPIC();

	// io 인터럽트 지정 엔트리 중에서 isa 버스와 관련된 인터럽트만 추려서 i/o 리다이렉션 테이블에 설정
	// mp 설정 테이블 헤더의 시작 어드레스와 엔트리의 시작 어드레스를 저장
	pstMPManager = kGetMPConfigurationManager();
	pstMPHeader = pstMPManager->pstMPConfigurationTableHeader;
	qwEntryAddress = pstMPManager->qwBaseEntryStartAddress;

	// 모든 엔트리를 확인하여 isa 버스와 관련된 i/o 인터럽트 지정 엔트리를 검색
	for(i=0; i<pstMPHeader->wEntryCount; i++){
		bEntryType = *(BYTE*)qwEntryAddress;
		switch(bEntryType){
			case MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT:
				pstIOAssignmentEntry = (IOINTERRUPTASSIGNMENTENTRY*)qwEntryAddress;
				// 인터럽트 타입이 인터럽트인 것만 처리
				if((pstIOAssignmentEntry->bSourceBUSID == pstMPManager->bISABusID) && 
					(pstIOAssignmentEntry->bInterruptType == MP_INTERRUPT_TYPE_INT)){
					if(pstIOAssignmentEntry->bSourceBUSIRQ == 0)
						bDestination = 0xff;
					else
						bDestination = 0x00;

					// isa 버스는 엣지 트리거와 1일때 활성화를 사용
					// 목적지 모드는 물리모드, 전달모드는 고정 모드
					// 인터럽트 벡터는 pic 컨트롤러의 백터와 같이 0x20+irq 설정
					kSetIOAPICRedirectionEntry(&stIORedirectionEntry, bDestination, 0x00, 
						IOAPIC_TRIGGERMODE_EDGE | IOAPIC_POLARITY_ACTIVEHIGH | IOAPIC_DESTINATIONMODE_PHYSICALMODE |
						IOAPIC_DELIVERYMODE_FIXED, PIC_IRQSTARTVECTOR + pstIOAssignmentEntry->bSourceBUSIRQ);

					// isa 버스에서 전달된 irq는 i/o apic 의 intin 핀에 있으므로, intin 값을 이용하여 처리
					kWriteIOAPICRedirectionTable(pstIOAssignmentEntry->bDestinationIOAPICINTIN, &stIORedirectionEntry);

					// irq와 인터럽트 입력핀의 관계를 저장
					gs_stIOAPICManager.vbIRQToINTINMap[pstIOAssignmentEntry->bSourceBUSIRQ] = pstIOAssignmentEntry->bDestinationIOAPICINTIN;

				}
				qwEntryAddress += sizeof(IOINTERRUPTASSIGNMENTENTRY);
				break;
			case MP_ENTRYTYPE_PROCESSOR:
				qwEntryAddress += sizeof(PROCESSORENTRY);
				break;
			case MP_ENTRYTYPE_BUS:
			case MP_ENTRYTYPE_IOAPIC:
			case MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT:
				qwEntryAddress += 8;
				break;
		}
	}
}

// irq와 i/o apic 의 인터럽트 핀 간의 매핑 관계를 출력
void kPrintIRQToINTINMap(void){
	int i;

	kPrintf("=========== irq to i/o apic int in mapping table =============\n");

	for(i=0; i<IOAPIC_MAXIRQTOINTINMAPCOUNT; i++)
		kPrintf("IRQ[%d] -> INTIN [%d]\n", i, gs_stIOAPICManager.vbIRQToINTINMap[i]);
}

// IRQ를 로컬 APIC ID로 전달하도록 변경
void kRoutingIRQToAPICID(int iIRQ, BYTE bAPICID){
	int i;
	IOREDIRECTIONTABLE stEntry;

	// 범위 검사
	if(iIRQ > IOAPIC_MAXIRQTOINTINMAPCOUNT)
		return;


	// 설정된 I/O 리다이렉션 테이블을 읽어서 목적지 필드만 수정
	kReadIOAPICRedirectionTable(gs_stIOAPICManager.vbIRQToINTINMap[iIRQ], &stEntry);
	stEntry.bDestination = bAPICID;
	kWriteIOAPICRedirectionTable(gs_stIOAPICManager.vbIRQToINTINMap[iIRQ], &stEntry);
}
