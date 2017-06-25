#ifndef __PAGE_H__
#define __PAGE_H__

#include "Types.h"

// ������ ���̺� ��Ʈ�� �ʵ�
#define PAGE_FLAGS_P   0x00000001 // (1 << 0)  : Present
#define PAGE_FLAGS_RW  0x00000002 // (1 << 1)  : Read/Write
#define PAGE_FLAGS_US  0x00000004 // (1 << 2)  : User/Supervisor
#define PAGE_FLAGS_PWT 0x00000008 // (1 << 3)  : Page-level Write Through
#define PAGE_FLAGS_PCD 0x00000010 // (1 << 4)  : Page-level Cache Disable
#define PAGE_FLAGS_A   0x00000020 // (1 << 5)  : Accessed
#define PAGE_FLAGS_D   0x00000040 // (1 << 6)  : Dirty
#define PAGE_FLAGS_PS  0x00000080 // (1 << 7)  : Page Size
#define PAGE_FLAGS_G   0x00000100 // (1 << 8)  : Global
#define PAGE_FLAGS_PAT 0x00001000 // (1 << 12) : Page Attribute Table Index
#define PAGE_FLAGS_EXB 0x80000000 // (1 << 31) : Excute Disable

// ���� ����� ��ũ��
#define PAGE_FLAGS_DEFAULT (PAGE_FLAGS_P | PAGE_FLAGS_RW)

// ��Ÿ ��ũ��
#define PAGE_TABLE_SIZE      0x1000     // 4KB
#define PAGE_MAX_ENTRY_COUNT 512
#define PAGE_DEFAULT_SIZE    0x200000   // 2MB

#pragma pack(push, 1)

typedef struct kPageTableEntryStruct {
	DWORD dwAttributeAndLowerBaseAddress;
	DWORD dwUpperBaseAddressAndEXB;
} PML4TENTRY, PDPTENTRY, PDENTRY, PTENTRY;

#pragma pack(pop)

void kInitializePageTables(void);
void kSetPageEntryData(PTENTRY* pstEntry, DWORD dwUpperBaseAddress, DWORD dwLowerBaseAddress, DWORD dwLowerFlags, DWORD dwUpperFlags);

#endif // __PAGE_H__
