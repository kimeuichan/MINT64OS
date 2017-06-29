#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include "Types.h"

/***** 매크로 정의 *****/
/* 빛의 3원색(RGB)
 * -빨간색(red)
 * -초록색(green)
 * -파란색(blue)
 *
 * 인쇄의 4원색(CMYK)
 * -청록색(cyan)
 * -자홍색(magenta)
 * -노란색(yellow)
 * -검정색(key)
 */
// 텍스트 모드 화면의 속성값
#define CONSOLE_BACKGROUND_BLACK         0x00 // 검정색
#define CONSOLE_BACKGROUND_BLUE          0x10 // 파란색
#define CONSOLE_BACKGROUND_GREEN         0x20 // 초록색
#define CONSOLE_BACKGROUND_CYAN          0x30 // 청록색(밝은 파란색)
#define CONSOLE_BACKGROUND_RED           0x40 // 빨간색
#define CONSOLE_BACKGROUND_MAGENTA       0x50 // 자홍색(보라색이 약간 섞인 빨간색)
#define CONSOLE_BACKGROUND_YELLOW        0x60 // 노란색
#define CONSOLE_BACKGROUND_WHITE         0x70 // 흰색
#define CONSOLE_BACKGROUND_BLINK         0x80 // 비디오 컨트롤러의 속성 모드 제어 레지스터의 Blink비트: 1->깜빡임 효과, 0->배경색 강조 효과(밝기 상승 효과)
#define CONSOLE_FOREGROUND_DARKBLACK     0x00 // 어두운 검정색
#define CONSOLE_FOREGROUND_DARKBLUE      0x01 // 어두운 파란색
#define CONSOLE_FOREGROUND_DARKGREEN     0x02 // 어두운 초록색
#define CONSOLE_FOREGROUND_DARKCYAN      0x03 // 어두운 청록색
#define CONSOLE_FOREGROUND_DARKRED       0x04 // 어두운 빨간색
#define CONSOLE_FOREGROUND_DARKMAGENTA   0x05 // 어두운 자홍색
#define CONSOLE_FOREGROUND_DARKYELLOW    0x06 // 어두운 노란색
#define CONSOLE_FOREGROUND_DARKWHITE     0x07 // 어두운 흰색
#define CONSOLE_FOREGROUND_BRIGHTBLACK   0x08 // 밝은 검정색
#define CONSOLE_FOREGROUND_BRIGHTBLUE    0x09 // 밝은 파란색
#define CONSOLE_FOREGROUND_BRIGHTGREEN   0x0A // 밝은 초록색
#define CONSOLE_FOREGROUND_BRIGHTCYAN    0x0B // 밝은 청록색
#define CONSOLE_FOREGROUND_BRIGHTRED     0x0C // 밝은 빨간색
#define CONSOLE_FOREGROUND_BRIGHTMAGENTA 0x0D // 밝은 자홍색
#define CONSOLE_FOREGROUND_BRIGHTYELLOW  0x0E // 밝은 노란색
#define CONSOLE_FOREGROUND_BRIGHTWHITE   0x0F // 밝은 흰색

// 기본 문자 색상
#define CONSOLE_DEFAULTTEXTCOLOR (CONSOLE_BACKGROUND_BLACK | CONSOLE_FOREGROUND_BRIGHTGREEN)

// 기타 콘솔 관련 매크로
#define CONSOLE_WIDTH              80
#define CONSOLE_HEIGHT             25
#define CONSOLE_VIDEOMEMORYADDRESS 0xB8000

// 비디오 컨트롤러 관련 매크로
#define VGA_PORT_INDEX        0x3D4 // CRTC 컨트롤 어드레스 레지스터
#define VGA_PORT_DATA         0x3D5 // CRTC 컨트롤 데이터 레지스터
#define VGA_INDEX_UPPERCURSOR 0x0E  // 상위 커서 위치 레지스터 선택 커맨드
#define VGA_INDEX_LOWERCURSOR 0x0F  // 하위 커서 위치 레지스터 선택 커맨드

/***** 구조체 정의 *****/
#pragma pack(push, 1)

typedef struct kConsoleManagerStruct{
	int iCurrentPrintOffset; // 문자/커서 출력 위치
} CONSOLEMANAGER;

#pragma pack(pop)

/***** 함수 정의 *****/
void kInitializeConsole(int iX, int iY);
void kSetCursor(int iX, int iY);
void kGetCursor(int* piX, int* piY);
void kPrintf(const char* pcFormatString, ...);
int kConsolePrintString(const char* pcBuffer);
void kClearScreen(void);
BYTE kGetCh(void);
void kPrintStringXY(int iX, int iY, const char* pcString);

#endif // __CONSOLE_H__
