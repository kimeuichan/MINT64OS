#include "Types.h"
#include "Page.h"
#include "ModeSwitch.h"

void kPrintString(int iX, int iY, const char *pcString);
BOOL kInitKernel64Area(void);
BOOL kIsMemoryEnough(void);
void kCopyKernel64ImageTo2Mbyte(void);

void Main( void )
{
	DWORD i;
	DWORD dwEAX, dwEBX, dwECX, dwEDX;
	char vcVendorString[13] = { 0, };

	kPrintString(0, 3, "Protected Mode C Language Kernel Started....[PASS]");

	// minimum memory size check
	kPrintString(0, 4, "Minimum Memory Size Check...................[    ]");
	if (kIsMemoryEnough() == FALSE)
	{
		kPrintString(45, 4, "FAIL");
		kPrintString(0, 5, "Not Enough Memory!! MINT64 OS Requires Over 64M Byte Memory!!");

		while (1);
	}
	else
	{
		kPrintString(45, 4, "PASS");
	}

	// init IA-32e mode kernel area
	kPrintString(0, 5, "IA-32e Kernel Area Initialize...............[    ]");
	if (kInitKernel64Area() == FALSE)
	{
		kPrintString(45, 5, "FAIL");
		kPrintString(0, 6, "Kernel Area Initalization Fail!!");

		while (1);
	}
	kPrintString(45, 5, "PASS");

	// create Page Table for IA-32e mode
	kPrintString(0, 6, "IA-32e Page Tables Initialize...............[    ]");
	kInitializePageTables();
	kPrintString(45, 6, "PASS");

	// read processor vendor information
	kPrintString(0, 7, "CPU Vendor Name.............................[            ]");

	kReadCPUID(0x00, &dwEAX, &dwEBX, &dwECX, &dwEDX);
	*(DWORD *)vcVendorString = dwEBX;
	*((DWORD *)vcVendorString + 1) = dwEDX;
	*((DWORD *)vcVendorString + 2) = dwECX;
	kPrintString(45, 7, vcVendorString);

	// check 64 bit support
	kPrintString(0, 8, "IA-32e Mode Support.........................[    ]");
	kReadCPUID(0x80000001, &dwEAX, &dwEBX, &dwECX, &dwEDX);
	if (dwEDX & (1 << 29))
	{
		kPrintString(45, 8, "PASS");
	}
	else
	{
		kPrintString(45, 8, "FAIL");
		kPrintString(0, 9, "This processor does not support 64bit mode");
		while (1);
	}

	kPrintString(0, 9, "Copy IA-32e Kernel To 2M Address............[    ]");
	kCopyKernel64ImageTo2Mbyte();
	kPrintString(45, 9, "PASS");

	kPrintString(0, 10, "Switch To IA-32e Mode");
	kSwitchAndExecute64bitKernel();

	while (1);
}

void kPrintString(int iX, int iY, const char *pcString)
{
	CHARACTER *pstScreen = (CHARACTER *)0xB8000;
	const char *p;

	pstScreen += (iY * 80) + iX;
	for (p = pcString; *p; ++p, ++pstScreen)
		pstScreen->bCharactor = *p;
}

BOOL kInitKernel64Area(void)
{
	DWORD *pdwCurAddr;

	// init start address 1MB
	pdwCurAddr = (DWORD *)0x100000;

	// loop unitl init end address 6MB
	while ((DWORD)pdwCurAddr < 0x600000)
	{
		*pdwCurAddr = 0x00;

		// if init fail, end
		if (*pdwCurAddr != 0)
			return FALSE;

		++pdwCurAddr;
	}

	return TRUE;
}

BOOL kIsMemoryEnough(void)
{
	DWORD *pdwCurAddr;

	// check start address 0x100000(1MB)
	pdwCurAddr = (DWORD *)0x100000;

	// check end address 0x4000000(64MB)
	while ((DWORD)pdwCurAddr < 0x4000000)
	{
		*pdwCurAddr = 0x12345678;

		if (*pdwCurAddr != 0x12345678)
			return FALSE;

		pdwCurAddr += (0x100000 / 4);
	}

	return TRUE;
}

void kCopyKernel64ImageTo2Mbyte(void)
{
	WORD wKernel32SectorCount, wTotalKernelSectorCount;
	DWORD *pdwSourceAddress, *pdwDestinationAddress;
	int i;

	// 0x7C05 is total kernel sector number, 0x7C07 is protected mode kernel sector number
	wTotalKernelSectorCount = *((WORD *)0x7C05);
	wKernel32SectorCount = *((WORD *)0x7C07);

	pdwSourceAddress = (DWORD *)(0x10000 + (wKernel32SectorCount * 512));
	pdwDestinationAddress = (DWORD *)0x200000;
	// copy IA-32e mode kernel sector size
	for (i = 0; i < 512 * (wTotalKernelSectorCount = wKernel32SectorCount) / 4; ++i)
	{
		*pdwDestinationAddress = *pdwSourceAddress;
		++pdwDestinationAddress;
		++pdwSourceAddress;
	}
}
