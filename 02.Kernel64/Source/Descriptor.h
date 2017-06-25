#ifndef __DESCRIPTOR_H__
#define __DESCRIPTOR_H__

#include "Types.h"

//====================================================================================================
// GDT/TSS 관련 매크로
//====================================================================================================
// 널/코드/데이터/TSS 세그먼트 디스크립터 필드
#define GDT_TYPE_CODE        0x0A // Code Segment (실행/읽기)
#define GDT_TYPE_DATA        0x02 // Data Segment (읽기/쓰기)
#define GDT_TYPE_TSS         0x09 // TSS Segment (Not Busy)
#define GDT_FLAGS_LOWER_S    0x10 // Descriptor Type (1:세그먼트 디스크립터, 0:시스템 디스크립터)
#define GDT_FLAGS_LOWER_DPL0 0x00 // Descriptor Privilege Level 0 (Highest, Kernel)
#define GDT_FLAGS_LOWER_DPL1 0x20 // Descriptor Privilege Level 1
#define GDT_FLAGS_LOWER_DPL2 0x40 // Descriptor Privilege Level 2
#define GDT_FLAGS_LOWER_DPL3 0x60 // Descriptor Privilege Level 3 (Lowest, User)
#define GDT_FLAGS_LOWER_P    0x80 // Present (1:현재 디스크립터가 유효함, 0:현재 디스트립터가 유효하지 않음)
#define GDT_FLAGS_UPPER_L    0x20 // IA-32e 모드에서 사용하는 필드 (1:IA-32e 모드의 64비트용 코드 세그먼트, 0:IA-32e 모드의 32비트 호환용 코드 세그먼트)
#define GDT_FLAGS_UPPER_DB   0x40 // Default Operation Size (1:32비트용 세그먼트, 0:16비트용 세그먼트)
#define GDT_FLAGS_UPPER_G    0x80 // Granularity (1:세그먼트 크기를 1MB(Limit 20비트)*4KB(가중치)=4GB 까지 설정 가능, 0:세그먼트 크기를 1MB(Limit 20비트) 까지 설정 가능)

// 자주 사용할 매크로
#define GDT_FLAGS_LOWER_KERNELCODE (GDT_TYPE_CODE | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_KERNELDATA (GDT_TYPE_DATA | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_TSS        (GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_USERCODE   (GDT_TYPE_CODE | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL3 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_USERDATA   (GDT_TYPE_DATA | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL3 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_UPPER_CODE       (GDT_FLAGS_UPPER_G | GDT_FLAGS_UPPER_L)
#define GDT_FLAGS_UPPER_DATA       (GDT_FLAGS_UPPER_G | GDT_FLAGS_UPPER_L)
#define GDT_FLAGS_UPPER_TSS        (GDT_FLAGS_UPPER_G)

// GDT의 기준 주소에서 세그먼트 디스크립터의 오프셋
#define GDT_KERNELCODESEGMENT 0x08
#define GDT_KERNELDATASEGMENT 0x10
#define GDT_TSSSEGMENT        0x18

// 기타 GDT에 관련된 매크로
#define GDTR_STARTADDRESS   0x142000
#define GDT_MAXENTRY8COUNT  3
#define GDT_MAXENTRY16COUNT 1
#define GDT_TABLESIZE       ((sizeof(GDTENTRY8) * GDT_MAXENTRY8COUNT) + (sizeof(GDTENTRY16) * GDT_MAXENTRY16COUNT))
#define TSS_SEGMENTSIZE     (sizeof(TSSSEGMENT))

//====================================================================================================
// IDT 관련 매크로
//====================================================================================================
// IDT 게이트 디스크립터 필드
#define IDT_TYPE_INTERRUPT 0x0E // Interrupt Gate
#define IDT_TYPE_TRAP      0x0F // Trap Date
#define IDT_FLAGS_DPL0     0x00 // Descriptor Privilege Level 0 (Highest, Kernel)
#define IDT_FLAGS_DPL1     0x20 // Descriptor Privilege Level 1
#define IDT_FLAGS_DPL2     0x40 // Descriptor Privilege Level 2
#define IDT_FLAGS_DPL3     0x60 // Descriptor Privilege Level 3 (Lowest, User)
#define IDT_FLAGS_P        0x80 // Present (1:현재 디스크립터가 유효함, 0:현재 디스트립터가 유효하지 않음)
#define IDT_FLAGS_IST0     0    // 기존 방식의 스택 스위칭 (기존방식:권한 변동이 있을 때만 스택 스위칭이 일어남)
#define IDT_FLAGS_IST1     1    // IST 방식의 스택 스위칭 (IST 방식:무조건 스택 스위칭이 일어남, IST1~7중에서 MINT64는 IST1만 이용)

// 자주 사용할 매크로
#define IDT_FLAGS_KERNEL (IDT_FLAGS_DPL0 | IDT_FLAGS_P)
#define IDT_FLAGS_USER   (IDT_FLAGS_DPL3 | IDT_FLAGS_P)

// 기타  IDT에 관련된 매크로
#define IDT_MAXENTRYCOUNT 100
#define IDTR_STARTADDRESS (GDTR_STARTADDRESS + sizeof(GDTR) + GDT_TABLESIZE + TSS_SEGMENTSIZE)
#define IDT_STARTADDRESS  (IDTR_STARTADDRESS + sizeof(IDTR))
#define IDT_TABLESIZE     (IDT_MAXENTRYCOUNT * sizeof(IDTENTRY))

// IST에 관련된 매크로
#define IST_STARTADDRESS 0x700000 // 7M
#define IST_SIZE         0x100000 // 1M

//====================================================================================================
// 자료구조
//====================================================================================================
#pragma pack(push, 1)

// GDTR/IDTR 자료구조 (16byte: 8byte의 배수인 16byte로 정렬하기 위해서 Padding Byte를 추가)
typedef struct kGDTRStruct{
	WORD wLimit;         // GDT/IDT Size
	QWORD qwBaseAddress; // GDT/IDT BaseAddress
	WORD wPadding;       // Padding Byte
	DWORD dwPadding;     // Padding Byte
} GDTR, IDTR;

// 널/코드/데이터 세그먼트 디스크립터 (8byte)
typedef struct kGDTEntry8Struct{   // 어셈블리 코드
	WORD wLowerLimit;              // dw 0xFFFF     ; Limit=0xFFFF
	WORD wLowerBaseAddress;        // dw 0x0000     ; BaseAddress=0x0000
	BYTE bUpperBaseAddress1;       // db 0x00       ; BaseAddress=0x00
	BYTE bTypeAndLowerFlags;       // db 0x9A|0x92  ; P=1, DPL=00, S=1, Type=0xA:CodeSegment(실행/읽기)|0x2:DataSegment(읽기/쓰기)
	BYTE bUpperLimitAndUpperFlags; // db 0xAF       ; G=1, D/B=0, L=1, AVL=0, Limit=0xF
	BYTE bUpperBaseAddress2;       // db 0x00       ; BaseAddress=0x00
} GDTENTRY8;

// TSS 세그먼트 디스크립터 (16byte)
typedef struct kGDTEntry16Struct{  // 어셈블리 코드
	WORD wLowerLimit;              // dw 0xFFFF     ; Limit=0xFFFF
	WORD wLowerBaseAddress;        // dw 0x0000     ; BaseAddress=0x0000
	BYTE bMiddleBaseAddress1;      // db 0x00       ; BaseAddress=0x00
	BYTE bTypeAndLowerFlags;       // db 0x99       ; P=1, DPL=00, S=1, Type=0x9:TSSSegment(NotBusy)
	BYTE bUpperLimitAndUpperFlags; // db 0xAF       ; G=1, D/B=0, L=0, AVL=0, Limit=0xF
	BYTE bMiddleBaseAddress2;      // db 0x00       ; BaseAddress=0x00
	DWORD dwUpperBaseAddress;      // dd 0x00000000 ; BaseAddress=0x00000000
	DWORD dwReserved;              // dd 0x00000000 ; Reserved=0x00000000
} GDTENTRY16;

// TSS 세그먼트 (104byte)
typedef struct kTSSDataStruct{
	DWORD dwReserved1;
	QWORD qwRsp[3];
	QWORD qwReserved2;
	QWORD qwIST[7];
	QWORD qwReserved3;
	WORD wReserved4;
	WORD wIOMapBaseAddress;
} TSSSEGMENT;

// IDT 게이트 디스크립터 (16byte)
typedef struct kIDTEntryStruct{ // 어셈블리 코드
	WORD wLowerBaseAddress;     // dw 0x????     ; HandlerOffset=0x????
	WORD wSegmentSelector;      // dw 0x0008     ; KernelCodeSegmentDescriptor=0x0008
	BYTE bIST;                  // db 0x01       ; Padding=00000, IST=001
	BYTE bTypeAndFlags;         // db 0x8E       ; P=1, DPL=00, Padding=0, Type=0xE:InterruptGate|0xF:TrapGate
	WORD wMiddleBaseAddress;    // dw 0x????     ; HandlerOffset=0x????
	DWORD dwUpperBaseAddress;   // dd 0x???????? ; HandlerOffset=0x????????
	DWORD dwReserved;           // dd 0x00000000 ; Reserved=0x00000000
}IDTENTRY;

#pragma pack(pop)

//====================================================================================================
// 함수
//====================================================================================================
void kInitializeGDTTableAndTSS(void);
void kSetGDTEntry8(GDTENTRY8* pstEntry, DWORD dwBaseAddress, DWORD dwLimit, BYTE bUpperFlags, BYTE bLowerFlags, BYTE bType);
void kSetGDTEntry16(GDTENTRY16* pstEntry, QWORD qwBaseAddress, DWORD dwLimit, BYTE bUpperFlags, BYTE bLowerFlags, BYTE bType);
void kInitializeTSSSegment(TSSSEGMENT* pstTSS);
void kInitializeIDTTable(void);
void kSetIDTEntry(IDTENTRY* pstEntry, void* pvHandler, WORD wSelector, BYTE bIST, BYTE bFlags, BYTE bType);
void kDummyHandler(void);

#endif // __DESCRIPTOR_H__
