#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include "Types.h"

/***** ��ũ�� ���� *****/
/* ���� 3����(RGB)
 * -������(red)
 * -�ʷϻ�(green)
 * -�Ķ���(blue)
 *
 * �μ��� 4����(CMYK)
 * -û�ϻ�(cyan)
 * -��ȫ��(magenta)
 * -�����(yellow)
 * -������(key)
 */
// �ؽ�Ʈ ��� ȭ���� �Ӽ���
#define CONSOLE_BACKGROUND_BLACK         0x00 // ������
#define CONSOLE_BACKGROUND_BLUE          0x10 // �Ķ���
#define CONSOLE_BACKGROUND_GREEN         0x20 // �ʷϻ�
#define CONSOLE_BACKGROUND_CYAN          0x30 // û�ϻ�(���� �Ķ���)
#define CONSOLE_BACKGROUND_RED           0x40 // ������
#define CONSOLE_BACKGROUND_MAGENTA       0x50 // ��ȫ��(������� �ణ ���� ������)
#define CONSOLE_BACKGROUND_YELLOW        0x60 // �����
#define CONSOLE_BACKGROUND_WHITE         0x70 // ���
#define CONSOLE_BACKGROUND_BLINK         0x80 // ���� ��Ʈ�ѷ��� �Ӽ� ��� ���� ���������� Blink��Ʈ: 1->������ ȿ��, 0->���� ���� ȿ��(��� ��� ȿ��)
#define CONSOLE_FOREGROUND_DARKBLACK     0x00 // ��ο� ������
#define CONSOLE_FOREGROUND_DARKBLUE      0x01 // ��ο� �Ķ���
#define CONSOLE_FOREGROUND_DARKGREEN     0x02 // ��ο� �ʷϻ�
#define CONSOLE_FOREGROUND_DARKCYAN      0x03 // ��ο� û�ϻ�
#define CONSOLE_FOREGROUND_DARKRED       0x04 // ��ο� ������
#define CONSOLE_FOREGROUND_DARKMAGENTA   0x05 // ��ο� ��ȫ��
#define CONSOLE_FOREGROUND_DARKYELLOW    0x06 // ��ο� �����
#define CONSOLE_FOREGROUND_DARKWHITE     0x07 // ��ο� ���
#define CONSOLE_FOREGROUND_BRIGHTBLACK   0x08 // ���� ������
#define CONSOLE_FOREGROUND_BRIGHTBLUE    0x09 // ���� �Ķ���
#define CONSOLE_FOREGROUND_BRIGHTGREEN   0x0A // ���� �ʷϻ�
#define CONSOLE_FOREGROUND_BRIGHTCYAN    0x0B // ���� û�ϻ�
#define CONSOLE_FOREGROUND_BRIGHTRED     0x0C // ���� ������
#define CONSOLE_FOREGROUND_BRIGHTMAGENTA 0x0D // ���� ��ȫ��
#define CONSOLE_FOREGROUND_BRIGHTYELLOW  0x0E // ���� �����
#define CONSOLE_FOREGROUND_BRIGHTWHITE   0x0F // ���� ���

// �⺻ ���� ����
#define CONSOLE_DEFAULTTEXTCOLOR (CONSOLE_BACKGROUND_BLACK | CONSOLE_FOREGROUND_BRIGHTGREEN)

// ��Ÿ �ܼ� ���� ��ũ��
#define CONSOLE_WIDTH              80
#define CONSOLE_HEIGHT             25
#define CONSOLE_VIDEOMEMORYADDRESS 0xB8000

// ���� ��Ʈ�ѷ� ���� ��ũ��
#define VGA_PORT_INDEX        0x3D4 // CRTC ��Ʈ�� ��巹�� ��������
#define VGA_PORT_DATA         0x3D5 // CRTC ��Ʈ�� ������ ��������
#define VGA_INDEX_UPPERCURSOR 0x0E  // ���� Ŀ�� ��ġ �������� ���� Ŀ�ǵ�
#define VGA_INDEX_LOWERCURSOR 0x0F  // ���� Ŀ�� ��ġ �������� ���� Ŀ�ǵ�

/***** ����ü ���� *****/
#pragma pack(push, 1)

typedef struct kConsoleManagerStruct{
	int iCurrentPrintOffset; // ����/Ŀ�� ��� ��ġ
} CONSOLEMANAGER;

#pragma pack(pop)

/***** �Լ� ���� *****/
void kInitializeConsole(int iX, int iY);
void kSetCursor(int iX, int iY);
void kGetCursor(int* piX, int* piY);
void kPrintf(const char* pcFormatString, ...);
int kConsolePrintString(const char* pcBuffer);
void kClearScreen(void);
BYTE kGetCh(void);
void kPrintStringXY(int iX, int iY, const char* pcString);

#endif // __CONSOLE_H__
