#include "Types.h"
#include "Utility.h"
#include "Keyboard.h"
#include "Descriptor.h"
#include "AssemblyUtility.h"
#include "PIC.h"

void Main(void){
	char vcTemp[2] = {0, };
	BOOL bFlags = FALSE;
	BYTE bTemp;
	int i = 0;

	// IA-32e 모드 C언어 커널 시작 메세지
	kPrintString(0, 10, "Switch to IA-32e Mode Success~!!");
	kPrintString(0, 11, "IA-32e Mode C Language Kernel Start.........[Pass]");

	// GDT/TSS 생성 및 GDT 로드
	kPrintString(0, 12, "GDT/TSS Initialize and GDT Load.............[    ]");
	kInitializeGDTTableAndTSS();
	kLoadGDTR(GDTR_STARTADDRESS);
	kPrintString(45, 12, "Pass");

	// TSS 로드
	kPrintString(0, 13, "TSS Load....................................[    ]");
	kLoadTR(GDT_TSSSEGMENT);
	kPrintString(45, 13, "Pass");

	// IDT 생성 및 로드
	kPrintString(0, 14, "IDT Initialize and Load.....................[    ]");
	kInitializeIDTTable();
	kLoadIDTR(IDTR_STARTADDRESS);
	kPrintString(45, 14, "Pass");

	// 키보드 활성화
	kPrintString(0, 15, "Keyboard Activate...........................[    ]");
	if(kActivateKeyboard() == TRUE){
		kPrintString(45, 15, "Pass");
		kChangeKeyboardLED(FALSE, FALSE, FALSE);

	}else{
		kPrintString(45, 15, "Fail");
		while(1);
	}

	// PIC 초기화 및 인터럽트 활성화
	kPrintString(0, 16, "PIC Initialize and Interrupt Activate.......[    ]");
	kInitializePIC();
	kMaskPICInterrupt(0);
	kEnableInterrupt();
	kPrintString(45, 16, "Pass");

	// 콘솔 쉘
	while(1){
		if(kIsOutputBufferFull() == TRUE){
			bTemp = kGetKeyboardScanCode();

			if(kConvertScanCodeToASCIICode(bTemp, &(vcTemp[0]), &bFlags) == TRUE){
				if(bFlags & KEY_FLAGS_DOWN){
					kPrintString(i++, 17, vcTemp);

					if(vcTemp[0] == '0'){
						// 벡터 0번 예외 발생
						bTemp = bTemp / 0;
					}
				}
			}
		}
	}
}
