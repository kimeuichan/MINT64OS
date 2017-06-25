#ifndef __DESCRIPTOR_H__
#define __DESCRIPTOR_H__

#include "Types.h"

//====================================================================================================
// GDT/TSS ���� ��ũ��
//====================================================================================================
// ��/�ڵ�/������/TSS ���׸�Ʈ ��ũ���� �ʵ�
#define GDT_TYPE_CODE        0x0A // Code Segment (����/�б�)
#define GDT_TYPE_DATA        0x02 // Data Segment (�б�/����)
#define GDT_TYPE_TSS         0x09 // TSS Segment (Not Busy)
#define GDT_FLAGS_LOWER_S    0x10 // Descriptor Type (1:���׸�Ʈ ��ũ����, 0:�ý��� ��ũ����)
#define GDT_FLAGS_LOWER_DPL0 0x00 // Descriptor Privilege Level 0 (Highest, Kernel)
#define GDT_FLAGS_LOWER_DPL1 0x20 // Descriptor Privilege Level 1
#define GDT_FLAGS_LOWER_DPL2 0x40 // Descriptor Privilege Level 2
#define GDT_FLAGS_LOWER_DPL3 0x60 // Descriptor Privilege Level 3 (Lowest, User)
#define GDT_FLAGS_LOWER_P    0x80 // Present (1:���� ��ũ���Ͱ� ��ȿ��, 0:���� ��Ʈ���Ͱ� ��ȿ���� ����)
#define GDT_FLAGS_UPPER_L    0x20 // IA-32e ��忡�� ����ϴ� �ʵ� (1:IA-32e ����� 64��Ʈ�� �ڵ� ���׸�Ʈ, 0:IA-32e ����� 32��Ʈ ȣȯ�� �ڵ� ���׸�Ʈ)
#define GDT_FLAGS_UPPER_DB   0x40 // Default Operation Size (1:32��Ʈ�� ���׸�Ʈ, 0:16��Ʈ�� ���׸�Ʈ)
#define GDT_FLAGS_UPPER_G    0x80 // Granularity (1:���׸�Ʈ ũ�⸦ 1MB(Limit 20��Ʈ)*4KB(����ġ)=4GB ���� ���� ����, 0:���׸�Ʈ ũ�⸦ 1MB(Limit 20��Ʈ) ���� ���� ����)

// ���� ����� ��ũ��
#define GDT_FLAGS_LOWER_KERNELCODE (GDT_TYPE_CODE | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_KERNELDATA (GDT_TYPE_DATA | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_TSS        (GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_USERCODE   (GDT_TYPE_CODE | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL3 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_USERDATA   (GDT_TYPE_DATA | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL3 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_UPPER_CODE       (GDT_FLAGS_UPPER_G | GDT_FLAGS_UPPER_L)
#define GDT_FLAGS_UPPER_DATA       (GDT_FLAGS_UPPER_G | GDT_FLAGS_UPPER_L)
#define GDT_FLAGS_UPPER_TSS        (GDT_FLAGS_UPPER_G)

// GDT�� ���� �ּҿ��� ���׸�Ʈ ��ũ������ ������
#define GDT_KERNELCODESEGMENT 0x08
#define GDT_KERNELDATASEGMENT 0x10
#define GDT_TSSSEGMENT        0x18

// ��Ÿ GDT�� ���õ� ��ũ��
#define GDTR_STARTADDRESS   0x142000
#define GDT_MAXENTRY8COUNT  3
#define GDT_MAXENTRY16COUNT 1
#define GDT_TABLESIZE       ((sizeof(GDTENTRY8) * GDT_MAXENTRY8COUNT) + (sizeof(GDTENTRY16) * GDT_MAXENTRY16COUNT))
#define TSS_SEGMENTSIZE     (sizeof(TSSSEGMENT))

//====================================================================================================
// IDT ���� ��ũ��
//====================================================================================================
// IDT ����Ʈ ��ũ���� �ʵ�
#define IDT_TYPE_INTERRUPT 0x0E // Interrupt Gate
#define IDT_TYPE_TRAP      0x0F // Trap Date
#define IDT_FLAGS_DPL0     0x00 // Descriptor Privilege Level 0 (Highest, Kernel)
#define IDT_FLAGS_DPL1     0x20 // Descriptor Privilege Level 1
#define IDT_FLAGS_DPL2     0x40 // Descriptor Privilege Level 2
#define IDT_FLAGS_DPL3     0x60 // Descriptor Privilege Level 3 (Lowest, User)
#define IDT_FLAGS_P        0x80 // Present (1:���� ��ũ���Ͱ� ��ȿ��, 0:���� ��Ʈ���Ͱ� ��ȿ���� ����)
#define IDT_FLAGS_IST0     0    // ���� ����� ���� ����Ī (�������:���� ������ ���� ���� ���� ����Ī�� �Ͼ)
#define IDT_FLAGS_IST1     1    // IST ����� ���� ����Ī (IST ���:������ ���� ����Ī�� �Ͼ, IST1~7�߿��� MINT64�� IST1�� �̿�)

// ���� ����� ��ũ��
#define IDT_FLAGS_KERNEL (IDT_FLAGS_DPL0 | IDT_FLAGS_P)
#define IDT_FLAGS_USER   (IDT_FLAGS_DPL3 | IDT_FLAGS_P)

// ��Ÿ  IDT�� ���õ� ��ũ��
#define IDT_MAXENTRYCOUNT 100
#define IDTR_STARTADDRESS (GDTR_STARTADDRESS + sizeof(GDTR) + GDT_TABLESIZE + TSS_SEGMENTSIZE)
#define IDT_STARTADDRESS  (IDTR_STARTADDRESS + sizeof(IDTR))
#define IDT_TABLESIZE     (IDT_MAXENTRYCOUNT * sizeof(IDTENTRY))

// IST�� ���õ� ��ũ��
#define IST_STARTADDRESS 0x700000 // 7M
#define IST_SIZE         0x100000 // 1M

//====================================================================================================
// �ڷᱸ��
//====================================================================================================
#pragma pack(push, 1)

// GDTR/IDTR �ڷᱸ�� (16byte: 8byte�� ����� 16byte�� �����ϱ� ���ؼ� Padding Byte�� �߰�)
typedef struct kGDTRStruct{
	WORD wLimit;         // GDT/IDT Size
	QWORD qwBaseAddress; // GDT/IDT BaseAddress
	WORD wPadding;       // Padding Byte
	DWORD dwPadding;     // Padding Byte
} GDTR, IDTR;

// ��/�ڵ�/������ ���׸�Ʈ ��ũ���� (8byte)
typedef struct kGDTEntry8Struct{   // ����� �ڵ�
	WORD wLowerLimit;              // dw 0xFFFF     ; Limit=0xFFFF
	WORD wLowerBaseAddress;        // dw 0x0000     ; BaseAddress=0x0000
	BYTE bUpperBaseAddress1;       // db 0x00       ; BaseAddress=0x00
	BYTE bTypeAndLowerFlags;       // db 0x9A|0x92  ; P=1, DPL=00, S=1, Type=0xA:CodeSegment(����/�б�)|0x2:DataSegment(�б�/����)
	BYTE bUpperLimitAndUpperFlags; // db 0xAF       ; G=1, D/B=0, L=1, AVL=0, Limit=0xF
	BYTE bUpperBaseAddress2;       // db 0x00       ; BaseAddress=0x00
} GDTENTRY8;

// TSS ���׸�Ʈ ��ũ���� (16byte)
typedef struct kGDTEntry16Struct{  // ����� �ڵ�
	WORD wLowerLimit;              // dw 0xFFFF     ; Limit=0xFFFF
	WORD wLowerBaseAddress;        // dw 0x0000     ; BaseAddress=0x0000
	BYTE bMiddleBaseAddress1;      // db 0x00       ; BaseAddress=0x00
	BYTE bTypeAndLowerFlags;       // db 0x99       ; P=1, DPL=00, S=1, Type=0x9:TSSSegment(NotBusy)
	BYTE bUpperLimitAndUpperFlags; // db 0xAF       ; G=1, D/B=0, L=0, AVL=0, Limit=0xF
	BYTE bMiddleBaseAddress2;      // db 0x00       ; BaseAddress=0x00
	DWORD dwUpperBaseAddress;      // dd 0x00000000 ; BaseAddress=0x00000000
	DWORD dwReserved;              // dd 0x00000000 ; Reserved=0x00000000
} GDTENTRY16;

// TSS ���׸�Ʈ (104byte)
typedef struct kTSSDataStruct{
	DWORD dwReserved1;
	QWORD qwRsp[3];
	QWORD qwReserved2;
	QWORD qwIST[7];
	QWORD qwReserved3;
	WORD wReserved4;
	WORD wIOMapBaseAddress;
} TSSSEGMENT;

// IDT ����Ʈ ��ũ���� (16byte)
typedef struct kIDTEntryStruct{ // ����� �ڵ�
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
// �Լ�
//====================================================================================================
void kInitializeGDTTableAndTSS(void);
void kSetGDTEntry8(GDTENTRY8* pstEntry, DWORD dwBaseAddress, DWORD dwLimit, BYTE bUpperFlags, BYTE bLowerFlags, BYTE bType);
void kSetGDTEntry16(GDTENTRY16* pstEntry, QWORD qwBaseAddress, DWORD dwLimit, BYTE bUpperFlags, BYTE bLowerFlags, BYTE bType);
void kInitializeTSSSegment(TSSSEGMENT* pstTSS);
void kInitializeIDTTable(void);
void kSetIDTEntry(IDTENTRY* pstEntry, void* pvHandler, WORD wSelector, BYTE bIST, BYTE bFlags, BYTE bType);
void kDummyHandler(void);

#endif // __DESCRIPTOR_H__
