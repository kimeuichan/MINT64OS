#include <stdarg.h>
#include "Console.h"
#include "Keyboard.h"
#include "Utility.h"
#include "Task.h"
#include "AssemblyUtility.h"

/***** ���� ���� ���� *****/
static CONSOLEMANAGER gs_stConsoleManager = {0, };

void kInitializeConsole(int iX, int iY){
	kMemSet(&gs_stConsoleManager, 0, sizeof(gs_stConsoleManager));
	kSetCursor(iX, iY);
}

void kSetCursor(int iX, int iY){
	int iLinearValue;

	iLinearValue = iY * CONSOLE_WIDTH + iX;

	// CRTC ��Ʈ�� ��巹�� ��������(0x3D4)�� ���� Ŀ�� ��ġ �������� ���� Ŀ�ǵ�(0x0E)�� ����
	kOutPortByte(VGA_PORT_INDEX, VGA_INDEX_UPPERCURSOR);

	// CRTC ��Ʈ�� ������ ��������(0x3D5)�� Ŀ���� ���� ����Ʈ�� ����
	kOutPortByte(VGA_PORT_DATA, iLinearValue >> 8);

	// CRTC ��Ʈ�� ��巹�� ��������(0x3D4)�� ���� Ŀ�� ��ġ �������� ���� Ŀ�ǵ�(0x0F)�� ����
	kOutPortByte(VGA_PORT_INDEX, VGA_INDEX_LOWERCURSOR);

	// CRTC ��Ʈ�� ������ ��������(0x3D5)�� Ŀ���� ���� ����Ʈ�� ����
	kOutPortByte(VGA_PORT_DATA, iLinearValue & 0xFF);

	gs_stConsoleManager.iCurrentPrintOffset = iLinearValue;
}

void kGetCursor(int* piX, int* piY){
	*piX = gs_stConsoleManager.iCurrentPrintOffset % CONSOLE_WIDTH;
	*piY = gs_stConsoleManager.iCurrentPrintOffset / CONSOLE_WIDTH;
}

void kPrintf(const char* pcFormatString, ...){
	va_list ap;
	char vcBuffer[1024];
	int iNextPrintOffset;

	va_start(ap, pcFormatString);
	kVSPrintf(vcBuffer, pcFormatString, ap);
	va_end(ap);

	iNextPrintOffset = kConsolePrintString(vcBuffer);

	kSetCursor(iNextPrintOffset % CONSOLE_WIDTH, iNextPrintOffset / CONSOLE_WIDTH);
}

int kConsolePrintString(const char* pcBuffer){
	CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
	int i, j;
	int iLength;
	int iPrintOffset;

	iPrintOffset = gs_stConsoleManager.iCurrentPrintOffset;

	iLength = kStrLen(pcBuffer);

	for(i = 0; i < iLength; i++){
		// �ٹٲ� ó��
		if(pcBuffer[i] == '\n'){
			// ��� ��ġ�� 80�� ��� ��ġ�� �̵� (���� ������ ù��° ��ġ�� �̵�)
			iPrintOffset += (CONSOLE_WIDTH - (iPrintOffset % CONSOLE_WIDTH));

		// �� ó��
		}else if(pcBuffer[i] == '\t'){
			// ��� ��ġ�� 8�� ��� ��ġ�� �̵� (���� ���� ù��° ��ġ�� �̵�)
			iPrintOffset += (8 - (iPrintOffset % 8));

		// �Ϲ� ���ڿ� ó��
		}else{
			pstScreen[iPrintOffset].bCharacter = pcBuffer[i];
			pstScreen[iPrintOffset].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
			iPrintOffset++;
		}

		// ��ũ�� ó��(��� ��ġ�� ȭ���� �ִ밪(80*25)�� ��� ���)
		if(iPrintOffset >= (CONSOLE_WIDTH * CONSOLE_HEIGHT)){

			// ���� �޸� ��ü(�ι�° �ٺ��� ������ �ٱ���)�� �� �� ���� ����
			kMemCpy((void*)CONSOLE_VIDEOMEMORYADDRESS
				   ,(void*)(CONSOLE_VIDEOMEMORYADDRESS + (CONSOLE_WIDTH * sizeof(CHARACTER)))
				   ,(CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH * sizeof(CHARACTER));

			// ������ ���� �������� ä��
			for(j = ((CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH); j < (CONSOLE_HEIGHT * CONSOLE_WIDTH); j++){
				pstScreen[j].bCharacter = ' ';
				pstScreen[j].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;

			}

			// ��� ��ġ�� ������ ���� ù��° ��ġ�� �̵�
			iPrintOffset = (CONSOLE_HEIGHT - 1) * CONSOLE_WIDTH;
		}
	}

	return iPrintOffset;
}

void kClearScreen(void){
	CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
	int i;

	for(i = 0; i < (CONSOLE_HEIGHT * CONSOLE_WIDTH); i++){
		pstScreen[i].bCharacter = ' ';
		pstScreen[i].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
	}

	kSetCursor(0, 0);
}

BYTE kGetCh(void){
	KEYDATA stData;

	// Ű�� ������ ������ ���
	while(1){

		// Ű ť�� �����Ͱ� ���ŵ� ������ ���
		while(kGetKeyFromKeyQueue(&stData) == FALSE){

			// Ű�� ����ϴ� ���� ���μ����� �ٸ� �½�ũ�� �纸�Ͽ� ���μ��� ������ ����
			kSchedule();
		}

		// Ű ť�� �����Ͱ� ���ŵǸ� �ƽ�Ű �ڵ带 ��ȯ
		if(stData.bFlags & KEY_FLAGS_DOWN){
			return stData.bASCIICode;
		}
	}
}

void kPrintStringXY(int iX, int iY, const char* pcString){
	CHARACTER* pstScreen = (CHARACTER*)CONSOLE_VIDEOMEMORYADDRESS;
	int i;

	pstScreen += (iY * CONSOLE_WIDTH) + iX;

	for(i = 0; pcString[i] != NULL; i++){
		pstScreen[i].bCharacter = pcString[i];
		pstScreen[i].bAttribute = CONSOLE_DEFAULTTEXTCOLOR;
	}
}
