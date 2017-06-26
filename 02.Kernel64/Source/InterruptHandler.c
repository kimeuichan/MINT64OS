#include "InterruptHandler.h"
#include "PIC.h"
// #include "Utility.h"
#include "Keyboard.h"

void kCommonExceptionHandler(int iVectorNumber, QWORD qwErrorCode){
	char vcBuffer[3] = {0, };

	// ���� ��ȣ ���� (2�ڸ� ����)
	vcBuffer[0] = '0' + iVectorNumber / 10;
	vcBuffer[1] = '0' + iVectorNumber % 10;

	kPrintString(0, 0, "==================================================");
	kPrintString(0, 1, "               Exception Occur~!!                 ");
	kPrintString(0, 2, "                  Vector:                         ");
	kPrintString(27, 2, vcBuffer); // "Vector:" ���ڿ� ���� ���
	kPrintString(0, 3, "==================================================");

	while(1);
}

void kCommonInterruptHandler(int iVectorNumber){
	char vcBuffer[] = "[INT:  , ]";
	static int g_iCommonInterruptCount = 0;

	//====================================================================================================
	// ���ͷ�Ʈ�� �߻������� �˸����� �޼����� ����ϴ� �κ�
	// ���� ��ȣ ���� (2�ڸ� ����)
	vcBuffer[5] = '0' + iVectorNumber / 10;
	vcBuffer[6] = '0' + iVectorNumber % 10;

	// �߻� Ƚ�� ���� (1�ڸ� ����)
	g_iCommonInterruptCount = (g_iCommonInterruptCount + 1) % 10;
	vcBuffer[8] = '0' + g_iCommonInterruptCount;

	// ȭ�� ������ ���� ���
	kPrintString(70, 0, vcBuffer);
	//====================================================================================================

	kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);
}

void kKeyboardHandler(int iVectorNumber){
	char vcBuffer[] = "[INT:  , ]";
	static int g_iKeyboardInterruptCount = 0;
	BYTE bTemp;

	//====================================================================================================
	// ���ͷ�Ʈ�� �߻������� �˸����� �޼����� ����ϴ� �κ�
	// ���� ��ȣ ���� (2�ڸ� ����)
	vcBuffer[5] = '0' + iVectorNumber / 10;
	vcBuffer[6] = '0' + iVectorNumber % 10;

	// �߻� Ƚ�� ���� (1�ڸ� ����)
	g_iKeyboardInterruptCount = (g_iKeyboardInterruptCount + 1) % 10;
	vcBuffer[8] = '0' + g_iKeyboardInterruptCount;

	// ȭ�� ���� ���� ���
	kPrintString(0, 0, vcBuffer);
	//====================================================================================================

	if(kIsOutputBufferFull() == TRUE){
		bTemp = kGetKeyboardScanCode();
		kConvertScanCodeAndPutQueue(bTemp);
	}
	kSendEOIToPIC(iVectorNumber - PIC_IRQSTARTVECTOR);
}
