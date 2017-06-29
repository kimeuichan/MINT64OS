#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include "Types.h"

/***** 매크로 정의 *****/
#define KEY_SKIPCOUNTFORPAUSE 2

// 키 상태 플래그
#define KEY_FLAGS_UP          0x00
#define KEY_FLAGS_DOWN        0x01
#define KEY_FLAGS_EXTENDEDKEY 0x02

#define KEY_MAPPINGTABLEMAXCOUNT 89

// 아스키 코드에 없는 키 값 (단, ENTER, TAB은 아스키 코드에 있음)
#define KEY_NONE        0x00
#define KEY_ENTER       '\n'
#define KEY_TAB         '\t'
#define KEY_ESC         0x1B
#define KEY_BACKSPACE   0x08
#define KEY_CTRL        0x81
#define KEY_LSHIFT      0x82
#define KEY_RSHIFT      0x83
#define KEY_PRINTSCREEN 0x84
#define KEY_LALT        0x85
#define KEY_CAPSLOCK    0x86
#define KEY_F1          0x87
#define KEY_F2          0x88
#define KEY_F3          0x89
#define KEY_F4          0x8A
#define KEY_F5          0x8B
#define KEY_F6          0x8C
#define KEY_F7          0x8D
#define KEY_F8          0x8E
#define KEY_F9          0x8F
#define KEY_F10         0x90
#define KEY_NUMLOCK     0x91
#define KEY_SCROLLLOCK  0x92
#define KEY_HOME        0x93
#define KEY_UP          0x94
#define KEY_PAGEUP      0x95
#define KEY_LEFT        0x96
#define KEY_CENTER      0x97
#define KEY_RIGHT       0x98
#define KEY_END         0x99
#define KEY_DOWN        0x9A
#define KEY_PAGEDOWN    0x9B
#define KEY_INS         0x9C
#define KEY_DEL         0x9D
#define KEY_F11         0x9E
#define KEY_F12         0x9F
#define KEY_PAUSE       0xA0

// 키 큐 관련 매크로
#define KEY_MAXQUEUECOUNT 100

/***** 구조체 정의 *****/
#pragma pack(push, 1)

typedef struct kKeyMappingEntryStruct{
	BYTE bNormalCode;   // 일반 키의 아스키 코드
	BYTE bCombinedCode; // 조합 키의 아스키 코드
} KEYMAPPINGENTRY;

typedef struct kKerboardManagerStruct{
	// 조합키 정보
	BOOL bShiftDown;
	BOOL bCapsLockOn;
	BOOL bNumLockOn;
	BOOL bScrollLockOn;

	// 확장키 정보
	BOOL bExtendedCodeIn;
	int iSkipCountForPause;
} KEYBOARDMANAGER;

typedef struct kKeyDataStruct{
	BYTE bScanCode;  // 스캔 코드
	BYTE bASCIICode; // 아스키 코드
	BYTE bFlags;     // 키 상태 플래그 (UP, DOWN, EXTENDEDKEY)
} KEYDATA;

#pragma pack(pop)

/***** 함수 정의 *****/
BOOL kIsOutputBufferFull(void);
BOOL kIsInputBufferFull(void);
BOOL kActivateKeyboard(void);
BYTE kGetKeyboardScanCode(void); // 스캔 코드 취득
BOOL kChangeKeyboardLED(BOOL bCapsLockOn, BOOL bNumLockOn, BOOL bScrollLockOn);
void kEnableA20Gate(void);
void kReboot(void);
BOOL kIsAlphabetScanCode(BYTE bDownScanCode);
BOOL kIsNumberOrSymbolScanCode(BYTE bDownScanCode);
BOOL kIsNumberPadScanCode(BYTE bDownScanCode);
BOOL kIsUseCombinedCode(BYTE bScanCode);
void kUpdateCombinationKeyStatusAndLED(BYTE bScanCode);
BOOL kConvertScanCodeToASCIICode(BYTE bScanCode, BYTE* pbASCIICode, BYTE* pbFlags);
BOOL kInitializeKeyboard(void);                   // 키 큐 초기화 및 키보드 활성화
BOOL kConvertScanCodeAndPutQueue(BYTE bScanCode); // 스캔 코드->아스키 코드 변환 및 키 큐에 데이터 삽입
BOOL kGetKeyFromKeyQueue(KEYDATA* pstData);       // 키 큐에서 데이터 삭제
BOOL kWaitForACKAndPutOtherScanCode(void);

#endif // __KEYBOARD_H__
