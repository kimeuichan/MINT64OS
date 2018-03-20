#include "MPConfigurationTable.h"
#include "Console.h"

// MP 설정 테이블을 관리하는 자료구조
static MPCONFIGURATIONMANAGER gs_stMPConfigurationManager ={0, };

// MP Floating 포인터 테이블을 찾는 함수
// MP Floating 포인터 테이블은 3 군데에 존재함 전부다 뒤져서 찾아햠
BOOL kFindMPFloatingPointerAddress( QWORD* pstAddress ){
    char* pcMPFloatingPointer;
    QWORD qwEBDAAddress;
    QWORD qwSystemBaseMemory;

    // 확장 BIOS 데이터 영역과 시스템 기본 메모리를 출력
    //kPrintf( "Extended BIOS Data Area = [0x%X] \n", 
    //        ( DWORD ) ( *( WORD* ) 0x040E ) * 16 );
    //kPrintf( "System Base Address = [0x%X]\n", 
    //        ( DWORD ) ( *( WORD* ) 0x0413 ) * 1024 );
    
    // 확장 BIOS 데이터 영역을 검색하여 MP 플로팅 포인터를 찾음
    // 확장 BIOS 데이터 영역은 0x040E에서 세그먼트의 시작 어드레스를 찾을 수 있음
    qwEBDAAddress = *( WORD* ) ( 0x040E );
    // 세그먼트의 시작 어드레스이므로 16을 곱하여 실제 물리 어드레스로 변환
    qwEBDAAddress *= 16;
    
    for( pcMPFloatingPointer = ( char* ) qwEBDAAddress ; 
         ( QWORD ) pcMPFloatingPointer <= ( qwEBDAAddress + 1024 ) ; 
         pcMPFloatingPointer++ )
    {
        if( kMemCmp( pcMPFloatingPointer, "_MP_", 4 ) == 0 )
        {
            //kPrintf( "MP Floating Pointer Is In EBDA, [0x%X] Address\n", 
            //         ( QWORD ) pcMPFloatingPointer );
            *pstAddress = ( QWORD ) pcMPFloatingPointer;
            return TRUE;
        }
    }

    // 시스템 기본 메모리의 끝부분에서 1Kbyte 미만인 영역을 검색하여 MP 플로팅 포인터를
    // 찾음
    // 시스템 기본 메모리는 0x0413에서 Kbyte 단위로 정렬된 값을 찾을 수 있음
    qwSystemBaseMemory = *( WORD* ) 0x0413;
    // Kbyte 단위로 저장된 값이므로 1024를 곱해 실제 물리 어드레스로 변환
    qwSystemBaseMemory *= 1024;

    for( pcMPFloatingPointer = ( char* ) ( qwSystemBaseMemory - 1024 ) ; 
        ( QWORD ) pcMPFloatingPointer <= qwSystemBaseMemory ; 
        pcMPFloatingPointer++ )
    {
        if( kMemCmp( pcMPFloatingPointer, "_MP_", 4 ) == 0 )
        {
            //kPrintf( "MP Floating Pointer Is In System Base Memory, [0x%X] Address\n",
            //         ( QWORD ) pcMPFloatingPointer );
            *pstAddress = ( QWORD ) pcMPFloatingPointer;
            return TRUE;
        }
    }
    
    // BIOS의 ROM 영역을 검색하여 MP 플로팅 포인터를 찾음
    for( pcMPFloatingPointer = ( char* ) 0x0F0000; 
         ( QWORD) pcMPFloatingPointer < 0x0FFFFF; pcMPFloatingPointer++ )
    {
        if( kMemCmp( pcMPFloatingPointer, "_MP_", 4 ) == 0 )
        {
            //kPrintf( "MP Floating Pointer Is In ROM, [0x%X] Address\n", 
            //         pcMPFloatingPointer );
            *pstAddress = ( QWORD ) pcMPFloatingPointer;
            return TRUE;
        }
    }

    return FALSE;
}

// MP 플로팅 테이블을 커널 자료구조에 삽입
BOOL kAnalysisMPConfigurationTable(void){
	QWORD qwMPFloatingPointerAddress;
	MPFLOATINGPOINTER* pstMPFloatingPointer;
	MPCONFIGURATIONTABLEHEADER* pstMPConfigurationTableHeader;
	BYTE bEntryType;
	WORD i;
	QWORD qwBaseEntryAddress;
	PROCESSORENTRY* pstProcessorEntry;
	BUSENTRY* pstBusEntry;

	kMemSet(&gs_stMPConfigurationManager, 0, sizeof(MPCONFIGURATIONMANAGER));
	gs_stMPConfigurationManager.bISABusID = 0xff;

	// MP 플로팅 테이블을 찾는 함수
	if(kFindMPFloatingPointerAddress(&qwMPFloatingPointerAddress) == FALSE)
		return FALSE;


	// MP 플로팅 포인터 설정
	pstMPFloatingPointer = (MPFLOATINGPOINTER*)qwMPFloatingPointerAddress;
	gs_stMPConfigurationManager.pstMPFloatingPointer = pstMPFloatingPointer;
	pstMPConfigurationTableHeader = (MPCONFIGURATIONTABLEHEADER*)((QWORD)pstMPFloatingPointer->dwMPConfigurationTableAddress & 0xffffffff);

	// PIC 모드 지원 여부 설정
	if(pstMPFloatingPointer->vbMPFeatureByte[1] & MP_FLOATINGPOINTER_FEATUREBYTE2_PICMODE)
		gs_stMPConfigurationManager.bUsePICMode = TRUE;

	// MP 설정 테이블 헤더, 기본 MP 설정 테이블 엔트리 시작 어드레스 설정
	gs_stMPConfigurationManager.pstMPConfigurationTableHeader = pstMPConfigurationTableHeader;
	gs_stMPConfigurationManager.qwBaseEntryStartAddress = pstMPFloatingPointer->dwMPConfigurationTableAddress + sizeof(MPCONFIGURATIONTABLEHEADER);

	// 프로세서/코어 개수, ISA 버스 ID 설정
	qwBaseEntryAddress = gs_stMPConfigurationManager.qwBaseEntryStartAddress;
	for(i=0; i<pstMPConfigurationTableHeader->wEntryCount; i++){
		bEntryType = *(BYTE*)qwBaseEntryAddress;
		switch(bEntryType){
			// 엔트리가 PROCESSOR 엔트리라면
			case MP_ENTRYTYPE_PROCESSOR:
				pstProcessorEntry = (PROCESSORENTRY*)qwBaseEntryAddress;
				// CPU 플래그가 사용 가능이면 증가
				if(pstProcessorEntry->bCPUFlags & MP_PROCESSOR_CPUFLAGS_ENABLE)
					gs_stMPConfigurationManager.iProcessorCount++;
				qwBaseEntryAddress += sizeof(PROCESSORENTRY);
				break;
			// 엔트리가 BUS 엔트리라면
			case MP_ENTRYTYPE_BUS:
				pstBusEntry = (BUSENTRY*)qwBaseEntryAddress;
				// type 스트링이 "ISA" 이면 버스아이디 설정
				if(kMemCmp(pstBusEntry->vcBusTypeString, MP_BUS_TYPESTRING_ISA, kStrLen(MP_BUS_TYPESTRING_ISA)) == 0)
					gs_stMPConfigurationManager.bISABusID = pstBusEntry->bBusID;
				qwBaseEntryAddress += sizeof(BUSENTRY);
				break;
			case MP_ENTRYTYPE_IOAPIC:
				qwBaseEntryAddress += sizeof(IOAPICENTRY);
				break;
			case MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT:
				qwBaseEntryAddress += sizeof(IOINTERRUPTASSIGNMENTENTRY);
				break;
			case MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT:
				qwBaseEntryAddress += sizeof(LOCALINTERRUPTASSIGNMENTENTRY);
				break;
			default:
				qwBaseEntryAddress += 8;
				break;
		}
	}
	return TRUE;
}

// MP 설정 테이블 관리 자료구조 반환 함수
MPCONFIGURATIONMANAGER* kGetMPConfigurationManager(void){
	return &gs_stMPConfigurationManager;
}

void kPrintMPConfigurationTable(void){
	MPCONFIGURATIONMANAGER* pstMPConfigurationManager;
	QWORD qwMPFloatingPointerAddress;
	MPFLOATINGPOINTER* pstMPFloatingPointer;
	MPCONFIGURATIONTABLEHEADER* pstMPConfigurationTableHeader;
	PROCESSORENTRY* pstProcessorEntry;
	BUSENTRY* pstBusEntry;
	IOAPICENTRY* pstIOAPICEntry;
	IOINTERRUPTASSIGNMENTENTRY* pstIOInterruptAssignmentEntry;
	LOCALINTERRUPTASSIGNMENTENTRY* pstLocalInterruptAssignmentEntry;
	QWORD qwBaseEntryAddress;
	char vcStringBuffer[20];
	WORD i;
	BYTE bEntryType;
	char* vpcInterruptType[4] = {"INT", "NMI", "SMI", "ExtInt"};
	char* vpcInterruptFlagsPO[4] = {"Conform", "Active High", "Reserved", "Active Low"};
	char* vpcInterruptFlagsEL[4] = {"Conform", "Edge-Trigger", "Reserved", "Level-Trigger"};

	// MP 설정 테이블 분석 함수 호출
	pstMPConfigurationManager = kGetMPConfigurationManager();
	if(pstMPConfigurationManager->qwBaseEntryStartAddress == 0 && kAnalysisMPConfigurationTable() == FALSE)
		return ;

	// MP 설정 테이블 매니저 정보 출력
	kPrintf("- MP Floating Pointer Address                     : 0x%Q\n", pstMPConfigurationManager->pstMPFloatingPointer);
	kPrintf("- MP Configuration Table Header Address           : 0x%Q\n", pstMPConfigurationManager->pstMPConfigurationTableHeader);
	kPrintf("- Base MP Configuration Table Entry Start Address : 0x%Q\n", pstMPConfigurationManager->qwBaseEntryStartAddress);
	kPrintf("- Processor/Core Count                            : %d\n", pstMPConfigurationManager->iProcessorCount);
	kPrintf("- PIC Mode Support                                : %s\n", (pstMPConfigurationManager->bUsePICMode == TRUE) ? "true" : "false");
	kPrintf("- ISA Bus ID                                      : %d\n", pstMPConfigurationManager->bISABusID);

	kPrintf("Press any key to continue...('q' is exit):");
	if(kGetCh() == 'q'){
		kPrintf("\n");
		return;
	}

	kPrintf("\n");

	// MP 플로팅 포인터 정보 출력
	kPrintf("\n====>>> MP Floating Pointer Info\n");
	pstMPFloatingPointer = gs_stMPConfigurationManager.pstMPFloatingPointer;

	kMemCpy(vcStringBuffer, pstMPFloatingPointer->vcSignature, sizeof(pstMPFloatingPointer->vcSignature));
	vcStringBuffer[sizeof(pstMPFloatingPointer->vcSignature)] = '\0';
	kPrintf("- Signature                      : %s\n", vcStringBuffer);
	kPrintf("- MP Configuration Table Address : 0x%Q\n", pstMPFloatingPointer->dwMPConfigurationTableAddress);
	kPrintf("- Length                         : %d bytes\n", pstMPFloatingPointer->bLength * 16);
	kPrintf("- Revision                       : %d\n", pstMPFloatingPointer->bRevision);
	kPrintf("- Checksum                       : 0x%X\n", pstMPFloatingPointer->bChecksum);
	kPrintf("- MP Feature Byte 1              : 0x%X ", pstMPFloatingPointer->vbMPFeatureByte[0]);
	if(pstMPFloatingPointer->vbMPFeatureByte[0] == MP_FLOATINGPOINTER_FEATUREBYTE1_USEMPTABLE)
		kPrintf("(Use MP Configuration Table)\n");
	else
		kPrintf("(Use Default Configuration)\n");
	kPrintf("- MP Feature Byte 2              : 0x%X ", pstMPFloatingPointer->vbMPFeatureByte[1]);
	if(pstMPFloatingPointer->vbMPFeatureByte[1] & MP_FLOATINGPOINTER_FEATUREBYTE2_PICMODE)
		kPrintf("(PIC Mode Support)\n");
	else
		kPrintf("(Virtual Wire Mode Support)\n");
	kPrintf("- MP Feature Byte 3              : 0x%X\n", pstMPFloatingPointer->vbMPFeatureByte[2]);
	kPrintf("- MP Feature Byte 4              : 0x%X\n", pstMPFloatingPointer->vbMPFeatureByte[3]);
	kPrintf("- MP Feature Byte 5              : 0x%X\n", pstMPFloatingPointer->vbMPFeatureByte[4]);

	kPrintf("Press any key to continue...('q' is exit):");
	if(kGetCh() == 'q'){
		kPrintf("\n");
		return;
	}

	kPrintf("\n");

	// MP 설정 테이블 헤더 정보 출력
	kPrintf("\n====>>>> MP Configuration Table Header Info\n");
	pstMPConfigurationTableHeader = gs_stMPConfigurationManager.pstMPConfigurationTableHeader;
	kMemCpy(vcStringBuffer, pstMPConfigurationTableHeader->vcSignature, sizeof(pstMPConfigurationTableHeader->vcSignature));
	vcStringBuffer[sizeof(pstMPConfigurationTableHeader->vcSignature)] = '\0';
	kPrintf("- Signature                           : %s\n", vcStringBuffer);
	kPrintf("- Base Table Length                   : %d bytes\n", pstMPConfigurationTableHeader->wBaseTableLength);
	kPrintf("- Revision                            : %d\n", pstMPConfigurationTableHeader->bRevision);
	kPrintf("- Checksum                            : 0x%X\n", pstMPConfigurationTableHeader->bChecksum);
	kMemCpy(vcStringBuffer, pstMPConfigurationTableHeader->vcOEMIDString, sizeof(pstMPConfigurationTableHeader->vcOEMIDString));
	vcStringBuffer[sizeof(pstMPConfigurationTableHeader->vcOEMIDString)] = '\0';
	kPrintf("- OEM ID String                       : %s\n", vcStringBuffer);
	kMemCpy(vcStringBuffer, pstMPConfigurationTableHeader->vcProductIDString, sizeof(pstMPConfigurationTableHeader->vcProductIDString));
	vcStringBuffer[sizeof(pstMPConfigurationTableHeader->vcProductIDString)] = '\0';
	kPrintf("- Product ID String                   : %s\n", vcStringBuffer);
	kPrintf("- OEM Table Pointer Address           : 0x%X\n", pstMPConfigurationTableHeader->dwOEMTablePointerAddress);
	kPrintf("- OEM Table Size                      : %d bytes\n", pstMPConfigurationTableHeader->wOEMTableSize);
	kPrintf("- Entry Count                         : %d\n", pstMPConfigurationTableHeader->wEntryCount);
	kPrintf("- Memory Map IO Address Of Local APIC : 0x%X\n", pstMPConfigurationTableHeader->dwMemoryMapIOAddressOfLocalAPIC);
	kPrintf("- Extended Table Length               : %d bytes\n", pstMPConfigurationTableHeader->wExtendedTableLength);
	kPrintf("- Extended Table Checksum             : 0x%X\n", pstMPConfigurationTableHeader->bExtendedTableChecksum);
	kPrintf("- Reserved                            : %d\n", pstMPConfigurationTableHeader->bReserved);

	kPrintf("Press any key to continue...('q' is exit):");
	if(kGetCh() == 'q'){
		kPrintf("\n");
		return;
	}

	kPrintf("\n");

	// MP 설정 테이블 엔트리 정보 출력
	kPrintf("\n====>>>> Base MP Configuration Table Entry Info (%d Entries)\n", pstMPConfigurationTableHeader->wEntryCount);
	qwBaseEntryAddress = pstMPConfigurationManager->qwBaseEntryStartAddress;
	for(i=0; i<pstMPConfigurationTableHeader->wEntryCount; i++){
		bEntryType = *(BYTE*)qwBaseEntryAddress;
		kPrintf("--------[Entry %d]----------------------------------------------------\n", i + 1);
		switch(bEntryType){
			case MP_ENTRYTYPE_PROCESSOR:
				pstProcessorEntry = (PROCESSORENTRY*)qwBaseEntryAddress;
				kPrintf("- Entry Type         : Processor Entry\n");
				kPrintf("- Local APIC ID      : %d\n", pstProcessorEntry->bLocalAPICID);
				kPrintf("- Local APIC Version : 0x%X\n", pstProcessorEntry->bLocalAPICVersion);
				kPrintf("- CPU Flags          : 0x%X ", pstProcessorEntry->bCPUFlags);
				if(pstProcessorEntry->bCPUFlags & MP_PROCESSOR_CPUFLAGS_ENABLE)
					kPrintf("(Enable, ");
				else
					kPrintf("(Disable, ");
				if(pstProcessorEntry->bCPUFlags & MP_PROCESSOR_CPUFLAGS_BSP)
					kPrintf("BSP)\n");
				else
					kPrintf("AP)\n");
				kPrintf("- CPU Signature      : 0x%X\n", pstProcessorEntry->vbCPUSignature);
				kPrintf("- Feature Flags      : 0x%X\n", pstProcessorEntry->dwFeatureFlags);
				kPrintf("- Reserved           : 0x%X\n", pstProcessorEntry->vdwReserved);

				qwBaseEntryAddress += sizeof(PROCESSORENTRY);
				break;

			case MP_ENTRYTYPE_BUS:
				pstBusEntry = (BUSENTRY*)qwBaseEntryAddress;
				kPrintf("- Entry Type      : Bus Entry\n");
			    kPrintf("- Bus ID          : %d\n", pstBusEntry->bBusID);
			    kMemCpy(vcStringBuffer, pstBusEntry->vcBusTypeString, sizeof(pstBusEntry->vcBusTypeString));
			    vcStringBuffer[sizeof(pstBusEntry->vcBusTypeString)] = '\0';
			    kPrintf("- Bus Type String : %s\n", vcStringBuffer);

				qwBaseEntryAddress += sizeof(BUSENTRY);
				break;
			case MP_ENTRYTYPE_IOAPIC:
				pstIOAPICEntry = (IOAPICENTRY*)qwBaseEntryAddress;
				kPrintf("- Entry Type            : IO APIC Entry\n");
				kPrintf("- IO APIC ID            : %d\n", pstIOAPICEntry->bIOAPICID);
				kPrintf("- IO APIC Version       : 0x%X\n", pstIOAPICEntry->bIOAPICVersion);
				kPrintf("- IO APIC Flags         : 0x%X ", pstIOAPICEntry->bIOAPICFlags);
				if(pstIOAPICEntry->bIOAPICFlags & MP_IOAPIC_FLAGS_ENABLE)
					kPrintf("(Enable)\n");
				else
					kPrintf("(Disable)\n");
				kPrintf("- Memory Map IO Address : 0x%X\n", pstIOAPICEntry->dwMemoryMapIOAddress);

				qwBaseEntryAddress += sizeof(IOAPICENTRY);
				break;
				case MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT:
				pstIOInterruptAssignmentEntry = (IOINTERRUPTASSIGNMENTENTRY*)qwBaseEntryAddress;

				kPrintf("- EntryType                 : IO Interrupt Assignment Entry\n");
				kPrintf("- Interrupt Type            : 0x%X ", pstIOInterruptAssignmentEntry->bInterruptType);
				kPrintf("(%s)\n", vpcInterruptType[pstIOInterruptAssignmentEntry->bInterruptType]);
				kPrintf("- Interrupt Flags           : 0x%X ", pstIOInterruptAssignmentEntry->wInterruptFlags);
				kPrintf("(%s, %s)\n", vpcInterruptFlagsPO[pstIOInterruptAssignmentEntry->wInterruptFlags & 0x03]
									, vpcInterruptFlagsEL[(pstIOInterruptAssignmentEntry->wInterruptFlags >> 2) & 0x03]);
				kPrintf("- Source BUS ID             : %d\n", pstIOInterruptAssignmentEntry->bSourceBUSID);
				kPrintf("- Source BUS IRQ            : %d\n", pstIOInterruptAssignmentEntry->bSourceBUSIRQ);
				kPrintf("- Destination IO APIC ID    : %d\n", pstIOInterruptAssignmentEntry->bDestinationIOAPICID);
				kPrintf("- Destination IO APIC INTIN : %d\n", pstIOInterruptAssignmentEntry->bDestinationIOAPICINTIN);

				qwBaseEntryAddress += sizeof(IOINTERRUPTASSIGNMENTENTRY);
				break;

			case MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT:
				pstLocalInterruptAssignmentEntry = (LOCALINTERRUPTASSIGNMENTENTRY*)qwBaseEntryAddress;

				kPrintf("- Entry Type                    : Local Interrupt Assignment Entry\n");
				kPrintf("- Interrupt Type                : 0x%X ", pstLocalInterruptAssignmentEntry->bInterruptType);
				kPrintf("(%s)\n", vpcInterruptType[pstLocalInterruptAssignmentEntry->bInterruptType]);
				kPrintf("- Interrupt Flags               : 0x%X ", pstLocalInterruptAssignmentEntry->wInterruptFlags);
				kPrintf("(%s, %s)\n", vpcInterruptFlagsPO[pstLocalInterruptAssignmentEntry->wInterruptFlags & 0x03]
									, vpcInterruptFlagsEL[(pstLocalInterruptAssignmentEntry->wInterruptFlags >> 2) & 0x03]);
				kPrintf("- Source BUS ID                 : %d\n", pstLocalInterruptAssignmentEntry->bSourceBUSID);
				kPrintf("- Source BUS IRQ                : %d\n", pstLocalInterruptAssignmentEntry->bSourceBUSIRQ);
				kPrintf("- Destination Local APIC ID     : %d\n", pstLocalInterruptAssignmentEntry->bDestinationLocalAPICID);
				kPrintf("- Destination Local APIC LINTIN : %d\n", pstLocalInterruptAssignmentEntry->bDestinationLocalAPICLINTIN);

				qwBaseEntryAddress += sizeof(LOCALINTERRUPTASSIGNMENTENTRY);
				break;

			default:
				kPrintf("Unknown Entry Type (%d)\n", bEntryType);
				break;
		}

		// 엔트리를 3개 출력시마다 엔트리를 더 출력할지 여부를 확인
		if((i != 0) && (((i + 1) % 3) == 0)){
			kPrintf("Press any key to continue...('q' is exit):");
			if(kGetCh() == 'q'){
				kPrintf("\n");
				return;
			}
			kPrintf("\n");
		}
	}
	kPrintf("--------[Entry End]---------------------------------------------------\n");
}

int kGetProcessorCount(void){
	// MP 설정 테이블이 없을수도 있으므로, 프로세서/코어 개수가 0인 경우 1을 반환
	if(gs_stMPConfigurationManager.iProcessorCount == 0)
		return 1;
	return gs_stMPConfigurationManager.iProcessorCount;
}

// isa 버스가 연결된 i/o apic 엔트리를 검색
// 		kAnalysisMPConfigurationTable() 함수를 먼저 호출한 뒤에 사용해야함
IOAPICENTRY* kFindIOAPICEntryForISA(void){
	MPCONFIGURATIONMANAGER* pstMPManager;
	MPCONFIGURATIONTABLEHEADER* pstMPHeader;
	IOINTERRUPTASSIGNMENTENTRY* pstIOAssignmentEntry;
	IOAPICENTRY* pstIOAPICEntry;
	QWORD qwEntryAddress;
	BYTE bEntryType;
	BOOL bFind = FALSE;
	int i;

	// mp 설정 테이블 헤더의 시작 어드레스와 엔트리의 시작 어드레스를 저장
	pstMPHeader = gs_stMPConfigurationManager.pstMPConfigurationTableHeader;
	qwEntryAddress = gs_stMPConfigurationManager.qwBaseEntryStartAddress;

	for(i=0; (i<pstMPHeader->wEntryCount) && (bFind == FALSE); i++){
		bEntryType = *(BYTE*)qwEntryAddress;
		switch(bEntryType){
			case MP_ENTRYTYPE_PROCESSOR:
				qwEntryAddress += sizeof(PROCESSORENTRY);
				break;
			case MP_ENTRYTYPE_BUS:
			case MP_ENTRYTYPE_IOAPIC:
			case MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT:
				qwEntryAddress += 8;
				break;

			case MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT:
				pstIOAssignmentEntry = (IOINTERRUPTASSIGNMENTENTRY*)qwEntryAddress;
				if(pstIOAssignmentEntry->bSourceBUSID == gs_stMPConfigurationManager.bISABusID)
					bFind = TRUE;
				qwEntryAddress += sizeof(IOINTERRUPTASSIGNMENTENTRY);
				break;

		}
	}

	if(bFind == FALSE)
		return NULL;

	qwEntryAddress = gs_stMPConfigurationManager.qwBaseEntryStartAddress;

	for(i=0; i<pstMPHeader->wEntryCount; i++){
		bEntryType = *(BYTE*)qwEntryAddress;
		switch(bEntryType){
			case MP_ENTRYTYPE_PROCESSOR:
				qwEntryAddress += sizeof(PROCESSORENTRY);
				break;
			case MP_ENTRYTYPE_BUS:
			case MP_ENTRYTYPE_IOINTERRUPTASSIGNMENT:
			case MP_ENTRYTYPE_LOCALINTERRUPTASSIGNMENT:
				qwEntryAddress += 8;
				break;

			case MP_ENTRYTYPE_IOAPIC:
				pstIOAPICEntry = (IOAPICENTRY*)qwEntryAddress;
				if(pstIOAPICEntry->bIOAPICID == pstIOAssignmentEntry->bDestinationIOAPICID)
					return pstIOAPICEntry;
				qwEntryAddress += sizeof(IOINTERRUPTASSIGNMENTENTRY);
				break;

		}
	}
	return NULL;
}