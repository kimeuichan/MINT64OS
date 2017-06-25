#include "Types.h"
#include "Page.h"
#include "ModeSwitch.h"

void kPrintString(int iX, int iY, const char* pcString);
BOOL kIsMemoryEnough(void);
BOOL kInitializeKernel64Area(void);
void kCopyKernel64ImageTo2Mbyte(void);

void Main(void){
	// 변수 선언
	DWORD dwEAX, dwEBX, dwECX, dwEDX;
	char vcVendorString[13] = {0, };

	// BootLoader.asm 의 메세지를 여기서 출력(임시방편) : 부트로더에서는 메세지 위치와 USB 파티션 정보 위치가 중복될 수도 있는 관계로 제거 했음.
	kPrintString(0, 0, "MINT64 OS Boot Loader Start~!!");
	kPrintString(0, 1, "OS Image Loading... Complete~!!");

	// 보호모드 C언어 커널 시작 메세지
	kPrintString(0, 3, "Protected Mode C Language Kernel Start......[Pass]");

	// 최소 메모리 크기 체크
	kPrintString(0, 4, "Minimum Memory Size Check...................[    ]");
	if(kIsMemoryEnough() == FALSE){
		kPrintString(45, 4, "Fail");
		kPrintString(0, 5, "Not Enough Memory~!! MINT64 OS Requires Over 64Mbytes Memory~!!");
		while(1);

	}else{
		kPrintString(45, 4, "Pass");
	}

	// IA-32e 모드 커널의 메모리 영역을 초기화
	kPrintString(0, 5, "IA-32e Kernel Area Initialize...............[    ]");
	if(kInitializeKernel64Area() == FALSE){
		kPrintString(45, 5, "Fail");
		kPrintString(0, 6, "Kernel Area Initialization Fail~!!");
		while(1);

	}else{
		kPrintString(45, 5, "Pass");
	}

	// IA-32e 모드 커널을 위한 페이지 테이블 생성
	kPrintString(0, 6, "IA-32e Page Tables Initialize...............[    ]");
	kInitializePageTables();
	kPrintString(45, 6, "Pass");

	// 프로세서 제조사 이름 읽기
	kReadCPUID(0x00000000, &dwEAX, &dwEBX, &dwECX, &dwEDX);
	*((DWORD*)vcVendorString) = dwEBX;
	*((DWORD*)vcVendorString + 1) = dwEDX;
	*((DWORD*)vcVendorString + 2) = dwECX;
	kPrintString(0, 7, "Processor Vendor String.....................[            ]");
	kPrintString(45, 7, vcVendorString);

	// 64비트 모드 지원 여부 확인
	kReadCPUID(0x80000001, &dwEAX, &dwEBX, &dwECX, &dwEDX);
	kPrintString(0, 8, "64bit Mode Support Check....................[    ]");
	if(dwEDX & (1 << 29)){
		kPrintString(45, 8, "Pass");

	}else{
		kPrintString(45, 8, "Fail");
		kPrintString(0, 9, "This processor does not support 64bit mode~!!");
		while(1);
	}

	// IA-32e 모드 커널을 0x200000(2Mbyte) 어드레스로 복사
	kPrintString(0, 9, "Copy IA-32e Kernel to 2MB Address...........[    ]");
	kCopyKernel64ImageTo2Mbyte();
	kPrintString(45, 9, "Pass");

	// IA-32e 모드 전환
	kSwitchAndExecute64bitKernel();

	while(1);
}

void kPrintString(int iX, int iY, const char* pcString){
	CHARACTER* pstScreen = (CHARACTER*)0xB8000;
	int i;

	pstScreen += (iY*80) + iX;

	for(i = 0; pcString[i] != NULL; i++){
		pstScreen[i].bCharacter = pcString[i];
	}

}

BOOL kIsMemoryEnough(void){
	DWORD* pdwCurrentAddress = (DWORD*)0x100000; // 1MB

	while((DWORD)pdwCurrentAddress < 0x4000000){ // 64MB
		*pdwCurrentAddress = 0x12345678;

		if(*pdwCurrentAddress != 0x12345678){
			return FALSE;
		}

		pdwCurrentAddress += (0x100000 / 4); // 1MB씩 증가
	}

	return TRUE;
}

BOOL kInitializeKernel64Area(void){
	DWORD* pdwCurrentAddress = (DWORD*)0x100000; // 1MB

	while((DWORD)pdwCurrentAddress < 0x600000){ // 6MB
		*pdwCurrentAddress = 0x00;

		if(*pdwCurrentAddress != 0x00){
			return FALSE;
		}

		pdwCurrentAddress++;
	}

	return TRUE;
}

void kCopyKernel64ImageTo2Mbyte(void){
	WORD wTotalSectorCount, wKernel32SectorCount;
	DWORD* pdwSrcAddress,* pdwDestAddress;
	int i;

	wTotalSectorCount = *((WORD*)0x7C05);
	wKernel32SectorCount = *((WORD*)0x7C07);

	pdwSrcAddress = (DWORD*)(0x10000 + (wKernel32SectorCount * 512));
	pdwDestAddress = (DWORD*)0x200000;

	for(i = 0; i < (((wTotalSectorCount - wKernel32SectorCount) * 512) / 4); i++){
		*pdwDestAddress = *pdwSrcAddress;
		pdwDestAddress++;
		pdwSrcAddress++;
	}
}
