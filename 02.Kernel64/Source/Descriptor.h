#ifndef __DESCRIPTOR_H__
#define __DESCRIPTOR_H__

#include "Types.h"

//====================================================================================================
// GDT/TSS °ü·Ã ¸ÅÅ©·Î
//====================================================================================================
// ³Î/ÄÚµå/µ¥ÀÌÅÍ/TSS ¼¼±×¸ÕÆ® µð½ºÅ©¸³ÅÍ ÇÊµå
#define GDT_TYPE_CODE        0x0A // Code Segment (½ÇÇà/ÀÐ±â)
#define GDT_TYPE_DATA        0x02 // Data Segment (ÀÐ±â/¾²±â)
#define GDT_TYPE_TSS         0x09 // TSS Segment (Not Busy)
#define GDT_FLAGS_LOWER_S    0x10 // Descriptor Type (1:¼¼±×¸ÕÆ® µð½ºÅ©¸³ÅÍ, 0:½Ã½ºÅÛ µð½ºÅ©¸³ÅÍ)
#define GDT_FLAGS_LOWER_DPL0 0x00 // Descriptor Privilege Level 0 (Highest, Kernel)
#define GDT_FLAGS_LOWER_DPL1 0x20 // Descriptor Privilege Level 1
#define GDT_FLAGS_LOWER_DPL2 0x40 // Descriptor Privilege Level 2
#define GDT_FLAGS_LOWER_DPL3 0x60 // Descriptor Privilege Level 3 (Lowest, User)
#define GDT_FLAGS_LOWER_P    0x80 // Present (1:ÇöÀç µð½ºÅ©¸³ÅÍ°¡ À¯È¿ÇÔ, 0:ÇöÀç µð½ºÆ®¸³ÅÍ°¡ À¯È¿ÇÏÁö ¾ÊÀ½)
#define GDT_FLAGS_UPPER_L    0x20 // IA-32e ¸ðµå¿¡¼­ »ç¿ëÇÏ´Â ÇÊµå (1:IA-32e ¸ðµåÀÇ 64ºñÆ®¿ë ÄÚµå ¼¼±×¸ÕÆ®, 0:IA-32e ¸ðµåÀÇ 32ºñÆ® È£È¯¿ë ÄÚµå ¼¼±×¸ÕÆ®)
#define GDT_FLAGS_UPPER_DB   0x40 // Default Operation Size (1:32ºñÆ®¿ë ¼¼±×¸ÕÆ®, 0:16ºñÆ®¿ë ¼¼±×¸ÕÆ®)
#define GDT_FLAGS_UPPER_G    0x80 // Granularity (1:¼¼±×¸ÕÆ® Å©±â¸¦ 1MB(Limit 20ºñÆ®)*4KB(°¡ÁßÄ¡)=4GB ±îÁö ¼³Á¤ °¡´É, 0:¼¼±×¸ÕÆ® Å©±â¸¦ 1MB(Limit 20ºñÆ®) ±îÁö ¼³Á¤ °¡´É)

// ÀÚÁÖ »ç¿ëÇÒ ¸ÅÅ©·Î
#define GDT_FLAGS_LOWER_KERNELCODE (GDT_TYPE_CODE | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_KERNELDATA (GDT_TYPE_DATA | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_TSS        (GDT_FLAGS_LOWER_DPL0 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_USERCODE   (GDT_TYPE_CODE | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL3 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_LOWER_USERDATA   (GDT_TYPE_DATA | GDT_FLAGS_LOWER_S | GDT_FLAGS_LOWER_DPL3 | GDT_FLAGS_LOWER_P)
#define GDT_FLAGS_UPPER_CODE       (GDT_FLAGS_UPPER_G | GDT_FLAGS_UPPER_L)
#define GDT_FLAGS_UPPER_DATA       (GDT_FLAGS_UPPER_G | GDT_FLAGS_UPPER_L)
#define GDT_FLAGS_UPPER_TSS        (GDT_FLAGS_UPPER_G)

// GDTÀÇ ±âÁØ ÁÖ¼Ò¿¡¼­ ¼¼±×¸ÕÆ® µð½ºÅ©¸³ÅÍÀÇ ¿ÀÇÁ¼Â
#define GDT_KERNELCODESEGMENT 0x08
#define GDT_KERNELDATASEGMENT 0x10
#define GDT_TSSSEGMENT        0x18

// ±âÅ¸ GDT¿¡ °ü·ÃµÈ ¸ÅÅ©·Î
#define GDTR_STARTADDRESS   0x142000
#define GDT_MAXENTRY8COUNT  3
// 16 바이트 엔트리의 개수, 즉 TSS 프로세서 또는 코어의 최대 개수 만큼 생성
#define GDT_MAXENTRY16COUNT (MAXPROCESSORCOUNT)
#define GDT_TABLESIZE       ((sizeof(GDTENTRY8) * GDT_MAXENTRY8COUNT) + (sizeof(GDTENTRY16) * GDT_MAXENTRY16COUNT))
#define TSS_SEGMENTSIZE     (sizeof(TSSSEGMENT)*MAXPROCESSORCOUNT)

//====================================================================================================
// IDT °ü·Ã ¸ÅÅ©·Î
//====================================================================================================
// IDT °ÔÀÌÆ® µð½ºÅ©¸³ÅÍ ÇÊµå
#define IDT_TYPE_INTERRUPT 0x0E // Interrupt Gate
#define IDT_TYPE_TRAP      0x0F // Trap Date
#define IDT_FLAGS_DPL0     0x00 // Descriptor Privilege Level 0 (Highest, Kernel)
#define IDT_FLAGS_DPL1     0x20 // Descriptor Privilege Level 1
#define IDT_FLAGS_DPL2     0x40 // Descriptor Privilege Level 2
#define IDT_FLAGS_DPL3     0x60 // Descriptor Privilege Level 3 (Lowest, User)
#define IDT_FLAGS_P        0x80 // Present (1:ÇöÀç µð½ºÅ©¸³ÅÍ°¡ À¯È¿ÇÔ, 0:ÇöÀç µð½ºÆ®¸³ÅÍ°¡ À¯È¿ÇÏÁö ¾ÊÀ½)
#define IDT_FLAGS_IST0     0    // ±âÁ¸ ¹æ½ÄÀÇ ½ºÅÃ ½ºÀ§Äª (±âÁ¸¹æ½Ä:±ÇÇÑ º¯µ¿ÀÌ ÀÖÀ» ¶§¸¸ ½ºÅÃ ½ºÀ§ÄªÀÌ ÀÏ¾î³²)
#define IDT_FLAGS_IST1     1    // IST ¹æ½ÄÀÇ ½ºÅÃ ½ºÀ§Äª (IST ¹æ½Ä:¹«Á¶°Ç ½ºÅÃ ½ºÀ§ÄªÀÌ ÀÏ¾î³², IST1~7Áß¿¡¼­ MINT64´Â IST1¸¸ ÀÌ¿ë)

// ÀÚÁÖ »ç¿ëÇÒ ¸ÅÅ©·Î
#define IDT_FLAGS_KERNEL (IDT_FLAGS_DPL0 | IDT_FLAGS_P)
#define IDT_FLAGS_USER   (IDT_FLAGS_DPL3 | IDT_FLAGS_P)

// ±âÅ¸  IDT¿¡ °ü·ÃµÈ ¸ÅÅ©·Î
#define IDT_MAXENTRYCOUNT 100
#define IDTR_STARTADDRESS (GDTR_STARTADDRESS + sizeof(GDTR) + GDT_TABLESIZE + TSS_SEGMENTSIZE)
#define IDT_STARTADDRESS  (IDTR_STARTADDRESS + sizeof(IDTR))
#define IDT_TABLESIZE     (IDT_MAXENTRYCOUNT * sizeof(IDTENTRY))

// IST¿¡ °ü·ÃµÈ ¸ÅÅ©·Î
#define IST_STARTADDRESS 0x700000 // 7M
#define IST_SIZE         0x100000 // 1M

//====================================================================================================
// ÀÚ·á±¸Á¶
//====================================================================================================
#pragma pack(push, 1)

// GDTR/IDTR ÀÚ·á±¸Á¶ (16byte: 8byteÀÇ ¹è¼öÀÎ 16byte·Î Á¤·ÄÇÏ±â À§ÇØ¼­ Padding Byte¸¦ Ãß°¡)
typedef struct kGDTRStruct{
	WORD wLimit;         // GDT/IDT Size
	QWORD qwBaseAddress; // GDT/IDT BaseAddress
	WORD wPadding;       // Padding Byte
	DWORD dwPadding;     // Padding Byte
} GDTR, IDTR;

// ³Î/ÄÚµå/µ¥ÀÌÅÍ ¼¼±×¸ÕÆ® µð½ºÅ©¸³ÅÍ (8byte)
typedef struct kGDTEntry8Struct{   // ¾î¼Àºí¸® ÄÚµå
	WORD wLowerLimit;              // dw 0xFFFF     ; Limit=0xFFFF
	WORD wLowerBaseAddress;        // dw 0x0000     ; BaseAddress=0x0000
	BYTE bUpperBaseAddress1;       // db 0x00       ; BaseAddress=0x00
	BYTE bTypeAndLowerFlags;       // db 0x9A|0x92  ; P=1, DPL=00, S=1, Type=0xA:CodeSegment(½ÇÇà/ÀÐ±â)|0x2:DataSegment(ÀÐ±â/¾²±â)
	BYTE bUpperLimitAndUpperFlags; // db 0xAF       ; G=1, D/B=0, L=1, AVL=0, Limit=0xF
	BYTE bUpperBaseAddress2;       // db 0x00       ; BaseAddress=0x00
} GDTENTRY8;

// TSS ¼¼±×¸ÕÆ® µð½ºÅ©¸³ÅÍ (16byte)
typedef struct kGDTEntry16Struct{  // ¾î¼Àºí¸® ÄÚµå
	WORD wLowerLimit;              // dw 0xFFFF     ; Limit=0xFFFF
	WORD wLowerBaseAddress;        // dw 0x0000     ; BaseAddress=0x0000
	BYTE bMiddleBaseAddress1;      // db 0x00       ; BaseAddress=0x00
	BYTE bTypeAndLowerFlags;       // db 0x99       ; P=1, DPL=00, S=1, Type=0x9:TSSSegment(NotBusy)
	BYTE bUpperLimitAndUpperFlags; // db 0xAF       ; G=1, D/B=0, L=0, AVL=0, Limit=0xF
	BYTE bMiddleBaseAddress2;      // db 0x00       ; BaseAddress=0x00
	DWORD dwUpperBaseAddress;      // dd 0x00000000 ; BaseAddress=0x00000000
	DWORD dwReserved;              // dd 0x00000000 ; Reserved=0x00000000
} GDTENTRY16;

// TSS ¼¼±×¸ÕÆ® (104byte)
typedef struct kTSSDataStruct{
	DWORD dwReserved1;
	QWORD qwRsp[3];
	QWORD qwReserved2;
	QWORD qwIST[7];
	QWORD qwReserved3;
	WORD wReserved4;
	WORD wIOMapBaseAddress;
} TSSSEGMENT;

// IDT °ÔÀÌÆ® µð½ºÅ©¸³ÅÍ (16byte)
typedef struct kIDTEntryStruct{ // ¾î¼Àºí¸® ÄÚµå
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
// ÇÔ¼ö
//====================================================================================================
void kInitializeGDTTableAndTSS(void);
void kSetGDTEntry8(GDTENTRY8* pstEntry, DWORD dwBaseAddress, DWORD dwLimit, BYTE bUpperFlags, BYTE bLowerFlags, BYTE bType);
void kSetGDTEntry16(GDTENTRY16* pstEntry, QWORD qwBaseAddress, DWORD dwLimit, BYTE bUpperFlags, BYTE bLowerFlags, BYTE bType);
void kInitializeTSSSegment(TSSSEGMENT* pstTSS);
void kInitializeIDTTable(void);
void kSetIDTEntry(IDTENTRY* pstEntry, void* pvHandler, WORD wSelector, BYTE bIST, BYTE bFlags, BYTE bType);
void kDummyHandler(void);

#endif // __DESCRIPTOR_H__
