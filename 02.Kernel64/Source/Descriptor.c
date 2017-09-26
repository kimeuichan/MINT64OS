#include "Descriptor.h"
#include "Utility.h"
#include "ISR.h"
#include "MultiProcessor.h"

//====================================================================================================
// GDT/TSS °ü·Ã ÇÔ¼ö
//====================================================================================================
void kInitializeGDTTableAndTSS(void){
	GDTR* pstGDTR;
	GDTENTRY8* pstEntry;
	TSSSEGMENT* pstTSS;
	int i;

	// GDTR »ý¼º
	pstGDTR = (GDTR*)GDTR_STARTADDRESS;
	pstEntry = (GDTENTRY8*)(GDTR_STARTADDRESS + sizeof(GDTR));
	pstGDTR->wLimit = GDT_TABLESIZE - 1;
	pstGDTR->qwBaseAddress = (QWORD)pstEntry;


	// TSS ¾îµå·¹½º ¼³Á¤
	pstTSS = (TSSSEGMENT*)((QWORD)pstEntry + GDT_TABLESIZE);

	// NULL, 64 비트 Code/Data, TSS를 위해 총 3+16 개의 세그먼트를 생성
	kSetGDTEntry8(&(pstEntry[0]), 0, 0, 0, 0, 0);
	kSetGDTEntry8(&(pstEntry[1]), 0x00000000, 0xFFFFF, GDT_FLAGS_UPPER_CODE, GDT_FLAGS_LOWER_KERNELCODE, GDT_TYPE_CODE);
	kSetGDTEntry8(&(pstEntry[2]), 0x00000000, 0xFFFFF, GDT_FLAGS_UPPER_DATA, GDT_FLAGS_LOWER_KERNELDATA, GDT_TYPE_DATA);

	for(i=0; i<MAXPROCESSORCOUNT; i++){
		// TSS는 16바이트이므로 kSetGDTEntry16 함수로 사용
		// pstEntry 는 8byte 이므로 2개를 합쳐서 하나로 사용
		kSetGDTEntry16((GDTENTRY16*)&(pstEntry[GDT_MAXENTRY8COUNT + 
			(i*2)]), (QWORD)pstTSS + (i*sizeof(TSSSEGMENT)), sizeof(TSSSEGMENT)-1, GDT_FLAGS_UPPER_TSS, GDT_FLAGS_LOWER_TSS, GDT_TYPE_TSS);

	}

	// TSS »ý¼º
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
	int i;

	// 최대 프로세서 또는 코어의 수만큼 루프를 돌면서 생성
	for(i=0; i<MAXPROCESSORCOUNT; i++){
		kMemSet(pstTSS, 0, sizeof(TSSSEGMENT));

		// IST 의 뒤에서부터 잘라서 할당함. (주의, IST는 16바이트 단위로 정렬해야함)
		pstTSS->qwIST[0] = IST_STARTADDRESS + IST_SIZE - (IST_SIZE / MAXPROCESSORCOUNT * i);

		// IO Map 기준 주소를 TSS 디스크립터의 limit 필드보다 크게 설정함으로써
		// IO Map 을 사용하지 않도록 함
		pstTSS->wIOMapBaseAddress = 0xffff;

		pstTSS++;
	}
}

//====================================================================================================
// IDT °ü·Ã ÇÔ¼ö
//====================================================================================================
void kInitializeIDTTable(void){
	IDTR* pstIDTR;
	IDTENTRY* pstEntry;
	int i;

	// IDTR »ý¼º
	pstIDTR = (IDTR*)IDTR_STARTADDRESS;
	pstEntry = (IDTENTRY*)(IDTR_STARTADDRESS + sizeof(IDTR));
	pstIDTR->wLimit = IDT_TABLESIZE - 1;
	pstIDTR->qwBaseAddress = (QWORD)pstEntry;

	// IDT(IDT °ÔÀÌÆ® µð½ºÅ©¸³ÅÍ 100°³) »ý¼º : 0~99±îÁöÀÇ º¤ÅÍ¿¡ ISR(Handler)¸¦ ¿¬°á
	// ¿¹¿Ü Ã³¸®¿ë ISR(21°³): #0~#19, #20~#31
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

	// ÀÎÅÍ·´Æ® Ã³¸®¿ë ISR(17°³): #32~#47, #48~#99
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
