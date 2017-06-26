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
	KEYDATA stData;

	// IA-32e ��� C��� Ŀ�� ���� �޼���
	kPrintString(0, 10, "Switch to IA-32e Mode Success~!!");
	kPrintString(0, 11, "IA-32e Mode C Language Kernel Start.........[Pass]");

	// GDT/TSS ���� �� GDT �ε�
	kPrintString(0, 12, "GDT/TSS Initialize and GDT Load.............[    ]");
	kInitializeGDTTableAndTSS();
	kLoadGDTR(GDTR_STARTADDRESS);
	kPrintString(45, 12, "Pass");

	// TSS �ε�
	kPrintString(0, 13, "TSS Load....................................[    ]");
	kLoadTR(GDT_TSSSEGMENT);
	kPrintString(45, 13, "Pass");

	// IDT ���� �� �ε�
	kPrintString(0, 14, "IDT Initialize and Load.....................[    ]");
	kInitializeIDTTable();
	kLoadIDTR(IDTR_STARTADDRESS);
	kPrintString(45, 14, "Pass");

	kPrintString(0, 15, "Key-Queue Initialize and Keyboard Activate..[    ]");
	if(kInitializeKeyboard() == TRUE){
		kPrintString(45, 15, "Pass");
		kChangeKeyboardLED(FALSE, FALSE, FALSE);
	}
	else{
		kPrintString(45, 15, "Fail");
		while(1);
	}

	// PIC �ʱ�ȭ �� ���ͷ�Ʈ Ȱ��ȭ
	kPrintString(0, 16, "PIC Initialize and Interrupt Activate.......[    ]");
	kInitializePIC();
	kMaskPICInterrupt(0);
	kEnableInterrupt();
	kPrintString(45, 16, "Pass");

	// �ܼ� ��
	while(1){
		if(kGetKeyFromKeyQueue(&stData) == TRUE){
			if(stData.bFlags & KEY_FLAGS_DOWN){
				vcTemp[0] = stData.bASCIICode;
				kPrintString(i++, 17, vcTemp);

				if(vcTemp[0] == '0')
					bTemp = bTemp / 0;
			}
		}
	}
}
