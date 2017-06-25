#include "Page.h"

void kInitializePageTables(void){
	PML4TENTRY* pstPML4TEnty;
	PDPTENTRY* pstPDPTEntry;
	PDENTRY* pstPDEntry;
	DWORD dwMappingAddress;
	int i;

	// PML4 테이블 생성(4KB의 메모리 차지) : 테이블 1개, 엔트리 1개
	pstPML4TEnty = (PML4TENTRY*)0x100000; // 1MB
	kSetPageEntryData(&(pstPML4TEnty[0]), 0x00, 0x101000, PAGE_FLAGS_DEFAULT, 0);
	for(i = 1; i < PAGE_MAX_ENTRY_COUNT; i++){
		kSetPageEntryData(&(pstPML4TEnty[i]), 0, 0, 0, 0);
	}

	// 페이지 디렉토리 포인터 테이블 생성(4KB의 메모리 차지) : 테이블 1개, 엔트리 64개
	pstPDPTEntry = (PDPTENTRY*)0x101000; // 1MB+4KB
	for(i = 0; i < 64; i++){
		kSetPageEntryData(&(pstPDPTEntry[i]), 0x00, 0x102000 + (i * PAGE_TABLE_SIZE), PAGE_FLAGS_DEFAULT, 0);
	}

	for(i = 64; i < PAGE_MAX_ENTRY_COUNT; i++){
		kSetPageEntryData(&(pstPDPTEntry[i]), 0, 0, 0, 0);
	}

	// 페이지 디렉토리 생성(4*64=256KB의 메모리 차지) : 테이블 64개, 엔트리  512*64=32768개
	// -페이지 테이블 총 개수 및 차지하는 메모리 사이즈 : 총 66개, 264KB의 메모리 차지
	// -지원 가능한 물리 메모리 사이즈 : 1GB*64=64GB
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
