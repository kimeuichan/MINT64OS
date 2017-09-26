#include "Types.h"
#include "Page.h"
#include "ModeSwitch.h"

#define DEF_FONT_C 0x07

void kPrintString(int iX, int iY, BYTE attr, const char* pcString);
BOOL kIsMemoryEnough(void);
BOOL kInitializeKernel64Area(void);
void kCopyKernel64ImageTo2Mbyte(void);

// Bootstrap Processor 여부가 저장된 어드레스, 부트 로더 영역의 앞쪽에 위치
#define BOOTSTRAPPROCESSOR_FLAGADDRESS	0x7c09

void Start32Kernel(void){
	// º¯¼ö ¼±¾ð
	DWORD dwEAX, dwEBX, dwECX, dwEDX;
	char vcVendorString[13] = {0, };

	if( *(BYTE*)BOOTSTRAPPROCESSOR_FLAGADDRESS == 0){
		kSwitchAndExecute64bitKernel();
		while(1);
	}

	// BootLoader.asm ÀÇ ¸Þ¼¼Áö¸¦ ¿©±â¼­ Ãâ·Â(ÀÓ½Ã¹æÆí) : ºÎÆ®·Î´õ¿¡¼­´Â ¸Þ¼¼Áö À§Ä¡¿Í USB ÆÄÆ¼¼Ç Á¤º¸ À§Ä¡°¡ Áßº¹µÉ ¼öµµ ÀÖ´Â °ü°è·Î Á¦°Å ÇßÀ½.
	kPrintString(0, 0,  DEF_FONT_C, "MINT64 OS Boot Loader Start~!!");
	kPrintString(0, 1,  DEF_FONT_C, "OS Image Loading... Complete~!!");

	// º¸È£¸ðµå C¾ð¾î Ä¿³Î ½ÃÀÛ ¸Þ¼¼Áö
	kPrintString(0, 3,  DEF_FONT_C, "Protected Mode C Language Kernel Start......[Pass]");

	// ÃÖ¼Ò ¸Þ¸ð¸® Å©±â Ã¼Å©
	kPrintString(0, 4,  DEF_FONT_C, "Minimum Memory Size Check...................[    ]");
	if(kIsMemoryEnough() == FALSE){
		kPrintString(45, 4, DEF_FONT_C,  "Fail");
		kPrintString(0, 5,  DEF_FONT_C, "Not Enough Memory~!! MINT64 OS Requires Over 64Mbytes Memory~!!");
		while(1);

	}else{
		kPrintString(45, 4, DEF_FONT_C,  "Pass");
	}

	// IA-32e ¸ðµå Ä¿³ÎÀÇ ¸Þ¸ð¸® ¿µ¿ªÀ» ÃÊ±âÈ­
	kPrintString(0, 5,  DEF_FONT_C, "IA-32e Kernel Area Initialize...............[    ]");
	if(kInitializeKernel64Area() == FALSE){
		kPrintString(45, 5, DEF_FONT_C,  "Fail");
		kPrintString(0, 6,  DEF_FONT_C, "Kernel Area Initialization Fail~!!");
		while(1);

	}else{
		kPrintString(45, 5, DEF_FONT_C,  "Pass");
	}

	// IA-32e ¸ðµå Ä¿³ÎÀ» À§ÇÑ ÆäÀÌÁö Å×ÀÌºí »ý¼º
	kPrintString(0, 6,  DEF_FONT_C, "IA-32e Page Tables Initialize...............[    ]");
	kInitializePageTables();
	kPrintString(45, 6, DEF_FONT_C,  "Pass");

	// ÇÁ·Î¼¼¼­ Á¦Á¶»ç ÀÌ¸§ ÀÐ±â
	kReadCPUID(0x00000000, &dwEAX, &dwEBX, &dwECX, &dwEDX);
	*((DWORD*)vcVendorString) = dwEBX;
	*((DWORD*)vcVendorString + 1) = dwEDX;
	*((DWORD*)vcVendorString + 2) = dwECX;
	kPrintString(0, 7,  DEF_FONT_C, "Processor Vendor String.....................[            ]");
	kPrintString(45, 7, DEF_FONT_C,  vcVendorString);

	// 64ºñÆ® ¸ðµå Áö¿ø ¿©ºÎ È®ÀÎ
	kReadCPUID(0x80000001, &dwEAX, &dwEBX, &dwECX, &dwEDX);
	kPrintString(0, 8,  DEF_FONT_C, "64bit Mode Support Check....................[    ]");
	if(dwEDX & (1 << 29)){
		kPrintString(45, 8, DEF_FONT_C,  "Pass");

	}else{
		kPrintString(45, 8, DEF_FONT_C,  "Fail");
		kPrintString(0, 9,  DEF_FONT_C, "This processor does not support 64bit mode~!!");
		while(1);
	}

	// IA-32e ¸ðµå Ä¿³ÎÀ» 0x200000(2Mbyte) ¾îµå·¹½º·Î º¹»ç
	kPrintString(0, 9,  DEF_FONT_C, "Copy IA-32e Kernel to 2MB Address...........[    ]");
	kCopyKernel64ImageTo2Mbyte();
	kPrintString(45, 9, DEF_FONT_C,  "Pass");

	// IA-32e ¸ðµå ÀüÈ¯
	kSwitchAndExecute64bitKernel();

	while(1);
}

void kPrintString(int iX, int iY, BYTE attr, const char* pcString){
	CHARACTER* pstScreen = (CHARACTER*)0xB8000;
	int i;

	pstScreen += (iY*80) + iX;

	for(i = 0; pcString[i] != NULL; i++){
		pstScreen[i].bCharacter = pcString[i];
		pstScreen[i].bAttribute = attr;
	}

}

BOOL kIsMemoryEnough(void){
	DWORD* pdwCurrentAddress = (DWORD*)0x100000; // 1MB

	while((DWORD)pdwCurrentAddress < 0x4000000){ // 64MB
		*pdwCurrentAddress = 0x12345678;

		if(*pdwCurrentAddress != 0x12345678){
			return FALSE;
		}

		pdwCurrentAddress += (0x100000 / 4); // 1MB¾¿ Áõ°¡
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
