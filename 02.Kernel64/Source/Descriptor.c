#include "Descriptor.h"
#include "Utility.h"
#include "ISR.h"

//====================================================================================================
// GDT/TSS 관련 함수
//====================================================================================================
void kInitializeGDTTableAndTSS(void){
	GDTR* pstGDTR;
	GDTENTRY8* pstEntry;
	TSSSEGMENT* pstTSS;

	// GDTR 생성
	pstGDTR = (GDTR*)GDTR_STARTADDRESS;
	pstEntry = (GDTENTRY8*)(GDTR_STARTADDRESS + sizeof(GDTR));
	pstGDTR->wLimit = GDT_TABLESIZE - 1;
	pstGDTR->qwBaseAddress = (QWORD)pstEntry;
	pstGDTR->wPadding = 0;
	pstGDTR->dwPadding = 0;

	// TSS 어드레스 설정
	pstTSS = (TSSSEGMENT*)((QWORD)pstEntry + GDT_TABLESIZE);

	// GDT(널/코드/데이터/TSS 세그먼트 디스크립터) 생성
	kSetGDTEntry8(&(pstEntry[0]), 0, 0, 0, 0, 0);
	kSetGDTEntry8(&(pstEntry[1]), 0x00000000, 0xFFFFF, GDT_FLAGS_UPPER_CODE, GDT_FLAGS_LOWER_KERNELCODE, GDT_TYPE_CODE);
	kSetGDTEntry8(&(pstEntry[2]), 0x00000000, 0xFFFFF, GDT_FLAGS_UPPER_DATA, GDT_FLAGS_LOWER_KERNELDATA, GDT_TYPE_DATA);
	kSetGDTEntry16((GDTENTRY16*)&(pstEntry[3]), (QWORD)pstTSS, sizeof(TSSSEGMENT) - 1, GDT_FLAGS_UPPER_TSS, GDT_FLAGS_LOWER_TSS, GDT_TYPE_TSS);

	// TSS 생성
	kInitializeTSSSegment(pstTSS);
}

void kSetGDTEntry8(GDTENTRY8* pstEntry, DWORD dwBaseAddress, DWORD dwLimit, BYTE bUpperFlags, BYTE bLowerFlags, BYTE bType){
	pstEntry->wLowerLimit = dwLimit & 0xFFFF;
	pstEntry->wLowerBaseAddress = dwBaseAddress & 0xFFFF;
	pstEntry->bUpperBaseAddress1 = (dwBaseAddress >> 16) & 0xFF;
	pstEntry->bTypeAndLowerFlags = bLowerFlags | bType;
	pstEntry->bUpperLimitAndUpperFlags = bUpperFlags | ((dwLimit >> 16) & 0xF);
	pstEntry->bUpperBaseAddress2 = (dwBaseAddress >> 24) & 0xFF;
}

void kSetGDTEntry16(GDTENTRY16* pstEntry, QWORD qwBaseAddress, DWORD dwLimit, BYTE bUpperFlags, BYTE bLowerFlags, BYTE bType){
	pstEntry->wLowerLimit = dwLimit & 0xFFFF;
	pstEntry->wLowerBaseAddress = qwBaseAddress & 0xFFFF;
	pstEntry->bMiddleBaseAddress1 = (qwBaseAddress >> 16) & 0xFF;
	pstEntry->bTypeAndLowerFlags = bLowerFlags | bType;
	pstEntry->bUpperLimitAndUpperFlags = bUpperFlags | ((dwLimit >> 16) & 0xF);
	pstEntry->bMiddleBaseAddress2 = (qwBaseAddress >> 24) & 0xFF;
	pstEntry->dwUpperBaseAddress = (qwBaseAddress >> 32) & 0xFFFFFFFF;
	pstEntry->dwReserved = 0;
}

void kInitializeTSSSegment(TSSSEGMENT* pstTSS){
	kMemSet(pstTSS, 0, sizeof(TSSSEGMENT));
	pstTSS->qwIST[0] = IST_STARTADDRESS + IST_SIZE;
	pstTSS->wIOMapBaseAddress = 0xFFFF; // IO를 TSS의 Limit값보다 크게 설정함으로써 IO Map을 사용하지 않도록 함
}

//====================================================================================================
// IDT 관련 함수
//====================================================================================================
void kInitializeIDTTable(void){
	IDTR* pstIDTR;
	IDTENTRY* pstEntry;
	int i;

	// IDTR 생성
	pstIDTR = (IDTR*)IDTR_STARTADDRESS;
	pstEntry = (IDTENTRY*)(IDTR_STARTADDRESS + sizeof(IDTR));
	pstIDTR->wLimit = IDT_TABLESIZE - 1;
	pstIDTR->qwBaseAddress = (QWORD)pstEntry;

	// IDT(IDT 게이트 디스크립터 100개) 생성 : 0~99까지의 벡터에 ISR(Handler)를 연결
	// 예외 처리용 ISR(21개): #0~#19, #20~#31
	kSetIDTEntry(&(pstEntry[0]),  kISRDivideError,               GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(pstEntry[1]),  kISRDebug,                     GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(pstEntry[2]),  kISRNMI,                       GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(pstEntry[3]),  kISRBreakPoint,                GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(pstEntry[4]),  kISROverflow,                  GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(pstEntry[5]),  kISRBoundRangeExceeded,        GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(pstEntry[6]),  kISRInvalidOpcode,             GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(pstEntry[7]),  kISRDeviceNotAvailable,        GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(pstEntry[8]),  kISRDoubleFault,               GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(pstEntry[9]),  kISRCoprocessorSegmentOverrun, GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(pstEntry[10]), kISRInvalidTSS,                GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(pstEntry[11]), kISRSegmentNotPresent,         GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(pstEntry[12]), kISRStackSegmentFault,         GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(pstEntry[13]), kISRGeneralProtection,         GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(pstEntry[14]), kISRPageFault,                 GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(pstEntry[15]), kISR15,                        GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(pstEntry[16]), kISRFPUError,                  GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(pstEntry[17]), kISRAlignmentCheck,            GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(pstEntry[18]), kISRMachineCheck,              GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(pstEntry[19]), kISRSIMDError,                 GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	for(i = 20; i < 32; i++){
		kSetIDTEntry(&(pstEntry[i]), kISRETCException,           GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	}

	// 인터럽트 처리용 ISR(17개): #32~#47, #48~#99
	kSetIDTEntry(&(pstEntry[32]), kISRTimer,                     GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(pstEntry[33]), kISRKeyboard,                  GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(pstEntry[34]), kISRSlavePIC,                  GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(pstEntry[35]), kISRSerial2,                   GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(pstEntry[36]), kISRSerial1,                   GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(pstEntry[37]), kISRParallel2,                 GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(pstEntry[38]), kISRFloppy,                    GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(pstEntry[39]), kISRParallel1,                 GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(pstEntry[40]), kISRRTC,                       GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(pstEntry[41]), kISRReserved,                  GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(pstEntry[42]), kISRNotUsed1,                  GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(pstEntry[43]), kISRNotUsed2,                  GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(pstEntry[44]), kISRMouse,                     GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(pstEntry[45]), kISRCoprocessor,               GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(pstEntry[46]), kISRHDD1,                      GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	kSetIDTEntry(&(pstEntry[47]), kISRHDD2,                      GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	for(i = 48; i < IDT_MAXENTRYCOUNT; i++){
		kSetIDTEntry(&(pstEntry[i]), kISRETCInterrupt,           GDT_KERNELCODESEGMENT, IDT_FLAGS_IST1, IDT_FLAGS_KERNEL, IDT_TYPE_INTERRUPT);
	}
}

void kSetIDTEntry(IDTENTRY* pstEntry, void* pvHandler, WORD wSelector, BYTE bIST, BYTE bFlags, BYTE bType){
	pstEntry->wLowerBaseAddress = (QWORD)pvHandler & 0xFFFF;
	pstEntry->wSegmentSelector = wSelector;
	pstEntry->bIST = bIST & 0x7;
	pstEntry->bTypeAndFlags = bFlags | bType;
	pstEntry->wMiddleBaseAddress = ((QWORD)pvHandler >> 16) & 0xFFFF;
	pstEntry->dwUpperBaseAddress = ((QWORD)pvHandler >> 32) & 0xFFFFFFFF;
	pstEntry->dwReserved = 0;
}
