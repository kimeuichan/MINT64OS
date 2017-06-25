#include "Page.h"

void kInitializePageTables(void){
	PML4TENTRY* pstPML4TEnty;
	PDPTENTRY* pstPDPTEntry;
	PDENTRY* pstPDEntry;
	DWORD dwMappingAddress;
	int i;

	// PML4 ���̺� ����(4KB�� �޸� ����) : ���̺� 1��, ��Ʈ�� 1��
	pstPML4TEnty = (PML4TENTRY*)0x100000; // 1MB
	kSetPageEntryData(&(pstPML4TEnty[0]), 0x00, 0x101000, PAGE_FLAGS_DEFAULT, 0);
	for(i = 1; i < PAGE_MAX_ENTRY_COUNT; i++){
		kSetPageEntryData(&(pstPML4TEnty[i]), 0, 0, 0, 0);
	}

	// ������ ���丮 ������ ���̺� ����(4KB�� �޸� ����) : ���̺� 1��, ��Ʈ�� 64��
	pstPDPTEntry = (PDPTENTRY*)0x101000; // 1MB+4KB
	for(i = 0; i < 64; i++){
		kSetPageEntryData(&(pstPDPTEntry[i]), 0x00, 0x102000 + (i * PAGE_TABLE_SIZE), PAGE_FLAGS_DEFAULT, 0);
	}

	for(i = 64; i < PAGE_MAX_ENTRY_COUNT; i++){
		kSetPageEntryData(&(pstPDPTEntry[i]), 0, 0, 0, 0);
	}

	// ������ ���丮 ����(4*64=256KB�� �޸� ����) : ���̺� 64��, ��Ʈ��  512*64=32768��
	// -������ ���̺� �� ���� �� �����ϴ� �޸� ������ : �� 66��, 264KB�� �޸� ����
	// -���� ������ ���� �޸� ������ : 1GB*64=64GB
	pstPDEntry = (PDENTRY*)0x102000; // 1MB+4KB+4KB
	dwMappingAddress = 0;
	for(i = 0; i < (PAGE_MAX_ENTRY_COUNT * 64); i++){
		kSetPageEntryData(&(pstPDEntry[i]), (i * (PAGE_DEFAULT_SIZE >> 20)) >> 12, dwMappingAddress, PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS, 0);
		dwMappingAddress += PAGE_DEFAULT_SIZE;
	}
}

void kSetPageEntryData(PTENTRY* pstEntry
		              ,DWORD dwUpperBaseAddress
					  ,DWORD dwLowerBaseAddress
					  ,DWORD dwLowerFlags
					  ,DWORD dwUpperFlags){
	pstEntry->dwAttributeAndLowerBaseAddress = dwLowerBaseAddress | dwLowerFlags;
	pstEntry->dwUpperBaseAddressAndEXB = (dwUpperBaseAddress & 0xFF) | dwUpperFlags;
}
