#include "Types.h"
#include "Page.h"
#include "ModeSwitch.h"

void kPrintString(int iX, int iY, const char* pcString);
BOOL kIsMemoryEnough(void);
BOOL kInitializeKernel64Area(void);
void kCopyKernel64ImageTo2Mbyte(void);

void Main(void){
	// ���� ����
	DWORD dwEAX, dwEBX, dwECX, dwEDX;
	char vcVendorString[13] = {0, };

	// BootLoader.asm �� �޼����� ���⼭ ���(�ӽù���) : ��Ʈ�δ������� �޼��� ��ġ�� USB ��Ƽ�� ���� ��ġ�� �ߺ��� ���� �ִ� ����� ���� ����.
	kPrintString(0, 0, "MINT64 OS Boot Loader Start~!!");
	kPrintString(0, 1, "OS Image Loading... Complete~!!");

	// ��ȣ��� C��� Ŀ�� ���� �޼���
	kPrintString(0, 3, "Protected Mode C Language Kernel Start......[Pass]");

	// �ּ� �޸� ũ�� üũ
	kPrintString(0, 4, "Minimum Memory Size Check...................[    ]");
	if(kIsMemoryEnough() == FALSE){
		kPrintString(45, 4, "Fail");
		kPrintString(0, 5, "Not Enough Memory~!! MINT64 OS Requires Over 64Mbytes Memory~!!");
		while(1);

	}else{
		kPrintString(45, 4, "Pass");
	}

	// IA-32e ��� Ŀ���� �޸� ������ �ʱ�ȭ
	kPrintString(0, 5, "IA-32e Kernel Area Initialize...............[    ]");
	if(kInitializeKernel64Area() == FALSE){
		kPrintString(45, 5, "Fail");
		kPrintString(0, 6, "Kernel Area Initialization Fail~!!");
		while(1);

	}else{
		kPrintString(45, 5, "Pass");
	}

	// IA-32e ��� Ŀ���� ���� ������ ���̺� ����
	kPrintString(0, 6, "IA-32e Page Tables Initialize...............[    ]");
	kInitializePageTables();
	kPrintString(45, 6, "Pass");

	// ���μ��� ������ �̸� �б�
	kReadCPUID(0x00000000, &dwEAX, &dwEBX, &dwECX, &dwEDX);
	*((DWORD*)vcVendorString) = dwEBX;
	*((DWORD*)vcVendorString + 1) = dwEDX;
	*((DWORD*)vcVendorString + 2) = dwECX;
	kPrintString(0, 7, "Processor Vendor String.....................[            ]");
	kPrintString(45, 7, vcVendorString);

	// 64��Ʈ ��� ���� ���� Ȯ��
	kReadCPUID(0x80000001, &dwEAX, &dwEBX, &dwECX, &dwEDX);
	kPrintString(0, 8, "64bit Mode Support Check....................[    ]");
	if(dwEDX & (1 << 29)){
		kPrintString(45, 8, "Pass");

	}else{
		kPrintString(45, 8, "Fail");
		kPrintString(0, 9, "This processor does not support 64bit mode~!!");
		while(1);
	}

	// IA-32e ��� Ŀ���� 0x200000(2Mbyte) ��巹���� ����
	kPrintString(0, 9, "Copy IA-32e Kernel to 2MB Address...........[    ]");
	kCopyKernel64ImageTo2Mbyte();
	kPrintString(45, 9, "Pass");

	// IA-32e ��� ��ȯ
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

		pdwCurrentAddress += (0x100000 / 4); // 1MB�� ����
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
