#include "Types.h"
#include "AssemblyUtility.h"
#include "Keyboard.h"
#include "Queue.h"
#include "Utility.h"
#include "Synchronization.h"

/* @키보드 컨트롤러의 레지스터와 포트 I/O 함수
 *  1.컨트롤 레지스터 : kOutPortByte(0x64, bData)
 *  2.상태 레지스터    : kInPortByte(0x64) : return bData
 *  3.입력 버퍼          : kOutPortByte(0x60, bData)
 *  4.출력 버퍼          : kInPortByte(0x60) : return bData
 */

BOOL kIsOutputBufferFull(void){

	// 상태 레지스터의 OUTB(비트 0)을 확인
	if(kInPortByte(0x64) & 0x01){
		return TRUE;
	}

	return FALSE;
}

BOOL kIsInputBufferFull(void){

	// 상태 레지스터의 INPB(비트 1)을 확인
	if(kInPortByte(0x64) & 0x02){
		return TRUE;
	}

	return FALSE;
}

BOOL kWaitForACKAndPutOtherScanCode(void){
	int i, j;
	BYTE bData;
	BOOL bResult = FALSE;

	for(j = 0; j < 100; j++){
		for(i = 0; i < 0xFFFF; i++){
			if(kIsOutputBufferFull() == TRUE){
				break;
			}
		}

		// 출력 버퍼에서 읽은 데이터가 [0xFA:ACK]인지 확인
		bData = kInPortByte(0x60);
		if(bData == 0xFA){
			bResult = TRUE;
			break;

		}else{
			kConvertScanCodeAndPutQueue(bData);
		}
	}

	return bResult;
}

BOOL kActivateKeyboard(void){
	int i, j;
	BOOL bPreviousFlag;
	BOOL bResult;

	bPreviousFlag = kSetInterruptFlag(FALSE);

	// 키보드 컨트롤러의 키보드 디바이스 활성화: 컨트롤 레지스터에 [0xAE:키보드 디바이스 활성화 커맨드]를 씀
	kOutPortByte(0x64, 0xAE);

	for(i = 0; i < 0xFFFF; i++){
		if(kIsInputBufferFull() == FALSE){
			break;
		}
	}

	// 키보드 활성화: 입력 버퍼에 [0xF4:키보드 활성화 커맨드]를 씀
	kOutPortByte(0x60, 0xF4);

	// 응답코드 확인: ACK을 수신할 때까지 대기함
	bResult = kWaitForACKAndPutOtherScanCode();

	kSetInterruptFlag(bPreviousFlag);

	return bResult;
}

BYTE kGetKeyboardScanCode(void){
	while(kIsOutputBufferFull() == FALSE){
		;
	}

	// 출력 버퍼에서 읽은 데이터(스캔 코드)를 반환
	return kInPortByte(0x60);
}

BOOL kChangeKeyboardLED(BOOL bCapsLockOn, BOOL bNumLockOn, BOOL bScrollLockOn){
	int i, j;
	BOOL bPreviousFlag;
	BOOL bResult;

	bPreviousFlag = kSetInterruptFlag(FALSE);

	for(i = 0; i < 0xFFFF; i++){
		if(kIsInputBufferFull() == FALSE){
			break;
		}
	}

	// 키보드 LED 상태 변경 커맨드 전송: 입력 버퍼에 [0xED:키보드 LED 상태 변경 커맨드]를 씀
	kOutPortByte(0x60, 0xED);

	for(i = 0; i < 0xFFFF; i++){
		if(kIsInputBufferFull() == FALSE){
			break;
		}
	}

	// 응답코드 확인: ACK을 수신할 때까지 대기함
	bResult = kWaitForACKAndPutOtherScanCode();

	if(bResult == FALSE){
		kSetInterruptFlag(bPreviousFlag);
		return FALSE;
	}


	// 키보드 LED 상태 변경 데이터 전송: 입력 버퍼에 [CapsLock(비트 2) | NumLock(비트 1) | ScrollLock(비트 0)]를 씀
	kOutPortByte(0x60, (bCapsLockOn << 2) | (bNumLockOn << 1) | (bScrollLockOn));

	for(i = 0; i < 0xFFFF; i++){
		if(kIsInputBufferFull() == FALSE){
			break;
		}
	}

	// 응답코드 확인: ACK을 수신할 때까지 대기함
	bResult = kWaitForACKAndPutOtherScanCode();

	kSetInterruptFlag(bPreviousFlag);

	return bResult;
}

void kEnableA20Gate(void){
	BYTE bOutputPortData; // 출력 포트 데이터
	int i;

	// 컨트롤 레지스터에 [0xD0:출력 포트 읽기 커맨드]를 씀
	kOutPortByte(0x64, 0xD0);

	for(i = 0; i < 0xFFFF; i++){
		if(kIsOutputBufferFull() == TRUE){
			break;
		}
	}

	// 출력 버퍼에서 데이터를 읽음
	bOutputPortData = kInPortByte(0x60);

	// A20 게이트 활성화 비트(비트 1)을 1로 설정
	bOutputPortData |= 0x02;

	for(i = 0; i < 0xFFFF; i++){
		if(kIsInputBufferFull() == FALSE){
			break;
		}
	}

	// 컨트롤 레지스터에 [0xD1:출력 포트 쓰기 커맨드]를 씀
	kOutPortByte(0x64, 0xD1);

	// 입력 버퍼에 데이터를 씀
	kOutPortByte(0x60, bOutputPortData);
}

void kReboot(void){
	int i;

	for(i = 0; i < 0xFFFF; i++){
		if(kIsInputBufferFull() == FALSE){
			break;
		}
	}

	// 컨트롤 레지스터에 [0xD1:출력 포트 쓰기 커맨드]를 씀
	kOutPortByte(0x64, 0xD1);

	// 프로세서 리셋 비트(비트 0)을 0로 설정
	kOutPortByte(0x60, 0x00);

	while(1){
		;
	}
}

///////////////////////////////////////////////
// 스캔 코드를  아스키 코드로 변환하는 기능에 관련된 함수들
///////////////////////////////////////////////

/***** 전역 변수 정의 *****/
// 키보드 상태를 관리하는 키보드 매니저
static KEYBOARDMANAGER gs_stKeyboardManager = {0, };

// 키 큐, 키 큐 버퍼
static QUEUE gs_stKeyQueue;
static KEYDATA gs_vstKeyQueueBuffer[KEY_MAXQUEUECOUNT];

// 스캔 코드를 아스키 코드로 변환하는 테이블
static KEYMAPPINGENTRY gs_vstKeyMappingTable[KEY_MAPPINGTABLEMAXCOUNT] = {
		//----------------------------------------------------------------
		//    Scan Code    |       ASCII Code
		//----------------------------------------------------------------
		//    Down  Up     | Normal          Combined
		//----------------------------------------------------------------
		/*  0:0x00, 0x80 */ {KEY_NONE,       KEY_NONE},
		/*  1:0x01, 0x81 */ {KEY_ESC,        KEY_ESC},
		/*  2:0x02, 0x82 */ {'1',            '!'},
		/*  3:0x03, 0x83 */ {'2',            '@'},
		/*  4:0x04, 0x84 */ {'3',            '#'},
		/*  5:0x05, 0x85 */ {'4',            '$'},
		/*  6:0x06, 0x86 */ {'5',            '%'},
		/*  7:0x07, 0x87 */ {'6',            '^'},
		/*  8:0x08, 0x88 */ {'7',            '&'},
		/*  9:0x09, 0x89 */ {'8',            '*'},
		/* 10:0x0A, 0x8A */ {'9',            '('},
		/* 11:0x0B, 0x8B */ {'0',            ')'},
		/* 12:0x0C, 0x8C */ {'-',            '_'},
		/* 13:0x0D, 0x8D */ {'=',            '+'},
		/* 14:0x0E, 0x8E */ {KEY_BACKSPACE,  KEY_BACKSPACE},
		/* 15:0x0F, 0x8F */ {KEY_TAB,        KEY_TAB},
		/* 16:0x10, 0x90 */ {'q',            'Q'},
		/* 17:0x11, 0x91 */ {'w',            'W'},
		/* 18:0x12, 0x92 */ {'e',            'E'},
		/* 19:0x13, 0x93 */ {'r',            'R'},
		/* 20:0x14, 0x94 */ {'t',            'T'},
		/* 21:0x15, 0x95 */ {'y',            'Y'},
		/* 22:0x16, 0x96 */ {'u',            'U'},
		/* 23:0x17, 0x97 */ {'i',            'I'},
		/* 24:0x18, 0x98 */ {'o',            'O'},
		/* 25:0x19, 0x99 */ {'p',            'P'},
		/* 26:0x1A, 0x9A */ {'[',            '{'},
		/* 27:0x1B, 0x9B */ {']',            '}'},
		/* 28:0x1C, 0x9C */ {KEY_ENTER,      KEY_ENTER},
		/* 29:0x1D, 0x9D */ {KEY_CTRL,       KEY_CTRL},
		/* 30:0x1E, 0x9E */ {'a',            'A'},
		/* 31:0x1F, 0x9F */ {'s',            'S'},
		/* 32:0x20, 0xA0 */ {'d',            'D'},
		/* 33:0x21, 0xA1 */ {'f',            'F'},
		/* 34:0x22, 0xA2 */ {'g',            'G'},
		/* 35:0x23, 0xA3 */ {'h',            'H'},
		/* 36:0x24, 0xA4 */ {'j',            'J'},
		/* 37:0x25, 0xA5 */ {'k',            'K'},
		/* 38:0x26, 0xA6 */ {'l',            'L'},
		/* 39:0x27, 0xA7 */ {';',            ':'},
		/* 40:0x28, 0xA8 */ {'\'',           '\"'},
		/* 41:0x29, 0xA9 */ {'`',            '~'},
		/* 42:0x2A, 0xAA */ {KEY_LSHIFT,     KEY_LSHIFT},
		/* 43:0x2B, 0xAB */ {'\\',           '|'},
		/* 44:0x2C, 0xAC */ {'z',            'Z'},
		/* 45:0x2D, 0xAD */ {'x',            'X'},
		/* 46:0x2E, 0xAE */ {'c',            'C'},
		/* 47:0x2F, 0xAF */ {'v',            'V'},
		/* 48:0x30, 0xB0 */ {'b',            'B'},
		/* 49:0x31, 0xB1 */ {'n',            'N'},
		/* 50:0x32, 0xB2 */ {'m',            'M'},
		/* 51:0x33, 0xB3 */ {',',            '<'},
		/* 52:0x34, 0xB4 */ {'.',            '>'},
		/* 53:0x35, 0xB5 */ {'/',            '?'},
		/* 54:0x36, 0xB6 */ {KEY_RSHIFT,     KEY_RSHIFT},
		/* 55:0x37, 0xB7 */ {'*',            '*'},
		/* 56:0x38, 0xB8 */ {KEY_LALT,       KEY_LALT},
		/* 57:0x39, 0xB9 */ {' ',            ' '},
		/* 58:0x3A, 0xBA */ {KEY_CAPSLOCK,   KEY_CAPSLOCK},
		/* 59:0x3B, 0xBB */ {KEY_F1,         KEY_F1},
		/* 60:0x3C, 0xBC */ {KEY_F2,         KEY_F2},
		/* 61:0x3D, 0xBD */ {KEY_F3,         KEY_F3},
		/* 62:0x3E, 0xBE */ {KEY_F4,         KEY_F4},
		/* 63:0x3F, 0xBF */ {KEY_F5,         KEY_F5},
		/* 64:0x40, 0xC0 */ {KEY_F6,         KEY_F6},
		/* 65:0x41, 0xC1 */ {KEY_F7,         KEY_F7},
		/* 66:0x42, 0xC2 */ {KEY_F8,         KEY_F8},
		/* 67:0x43, 0xC3 */ {KEY_F9,         KEY_F9},
		/* 68:0x44, 0xC4 */ {KEY_F10,        KEY_F10},
		/* 69:0x45, 0xC5 */ {KEY_NUMLOCK,    KEY_NUMLOCK},
		/* 70:0x46, 0xC6 */ {KEY_SCROLLLOCK, KEY_SCROLLLOCK},
		/* 71:0x47, 0xC7 */ {KEY_HOME,       '7'},
		/* 72:0x48, 0xC8 */ {KEY_UP,         '8'},
		/* 73:0x49, 0xC9 */ {KEY_PAGEUP,     '9'},
		/* 74:0x4A, 0xCA */ {'-',            '-'},
		/* 75:0x4B, 0xCB */ {KEY_LEFT,       '4'},
		/* 76:0x4C, 0xCC */ {KEY_CENTER,     '5'},
		/* 77:0x4D, 0xCD */ {KEY_RIGHT,      '6'},
		/* 78:0x4E, 0xCE */ {'+',            '+'},
		/* 79:0x4F, 0xCF */ {KEY_END,        '1'},
		/* 80:0x50, 0xD0 */ {KEY_DOWN,       '2'},
		/* 81:0x51, 0xD1 */ {KEY_PAGEDOWN,   '3'},
		/* 82:0x52, 0xD2 */ {KEY_INS,        '0'},
		/* 83:0x53, 0xD3 */ {KEY_DEL,        '.'},
		/* 84:0x54, 0xD4 */ {KEY_NONE,       KEY_NONE},
		/* 85:0x55, 0xD5 */ {KEY_NONE,       KEY_NONE},
		/* 86:0x56, 0xD6 */ {KEY_NONE,       KEY_NONE},
		/* 87:0x57, 0xD7 */ {KEY_F11,        KEY_F11},
		/* 88:0x58, 0xD8 */ {KEY_F12,        KEY_F12}
};

BOOL kIsAlphabetScanCode(BYTE bDownScanCode){
	// 스캔 코드의 아스키 코드가 [a~z]이면 알파벳임
	if(('a' <= gs_vstKeyMappingTable[bDownScanCode].bNormalCode) && (gs_vstKeyMappingTable[bDownScanCode].bNormalCode <= 'z')){
		return TRUE;
	}

	return FALSE;
}

BOOL kIsNumberOrSymbolScanCode(BYTE bDownScanCode){
	// 숫자 패드와 확장 키를 제외한 범위(스캔코드 2~53)에서 알파벳이 아니면 숫자나 기호임
	if((2 <= bDownScanCode) && (bDownScanCode <= 53) && (kIsAlphabetScanCode(bDownScanCode) == FALSE)){
		return TRUE;
	}

	return FALSE;
}

BOOL kIsNumberPadScanCode(BYTE bDownScanCode){
	// 스캔코드 71~83이 숫자 패드임
	if((71 <= bDownScanCode) && (bDownScanCode <= 83)){
		return TRUE;
	}

	return FALSE;
}

BOOL kIsUseCombinedCode(BYTE bScanCode){
	BYTE bDownScanCode;
	BOOL bUseCombinedKey = FALSE;

	bDownScanCode = bScanCode & 0x7F;

	// 알파벳키는 Shift키와 Caps Lock키의 영항을 받음
	if(kIsAlphabetScanCode(bDownScanCode) == TRUE){
		if(gs_stKeyboardManager.bShiftDown ^ gs_stKeyboardManager.bCapsLockOn){
			bUseCombinedKey = TRUE;

		}else{
			bUseCombinedKey = FALSE;
		}

	// 숫자나 기호키는 Shift키의 영향을 받음
	}else if(kIsNumberOrSymbolScanCode(bDownScanCode) == TRUE){
		if(gs_stKeyboardManager.bShiftDown == TRUE){
			bUseCombinedKey = TRUE;

		}else{
			bUseCombinedKey = FALSE;
		}

	// 숫자 패드키는 Num Lock키의 영향를 받음
	// AND 0xE0만 제외하면 확장키 코드와 숫자 패드키 코드가 겹치므로, 확장키 코드가 수신되지 않았을 때만 처리
	}else if((kIsNumberPadScanCode(bDownScanCode) == TRUE) && (gs_stKeyboardManager.bExtendedCodeIn == FALSE)){
		if(gs_stKeyboardManager.bNumLockOn == TRUE){
			bUseCombinedKey = TRUE;

		}else{
			bUseCombinedKey = FALSE;

		}
	}

	return bUseCombinedKey;
}

void kUpdateCombinationKeyStatusAndLED(BYTE bScanCode){
	BOOL bDown = FALSE;
	BYTE bDownScanCode;
	BOOL bLEDStatusChanged = FALSE;

	// 스캔 코드의 최상위 비트(비트 7)가 1이면 Up Code, 0이면 Down Code
	if(bScanCode & 0x80){
		bDown = FALSE;
		bDownScanCode = bScanCode & 0x7F;

	}else{
		bDown = TRUE;
		bDownScanCode = bScanCode;

	}

	// [42:Left Shift] OR [54:Right Shift]인 경우
	if((bDownScanCode == 42) || (bDownScanCode == 54)){
		gs_stKeyboardManager.bShiftDown = bDown;

	// [58:Caps Lock] AND Down Code
	}else if((bDownScanCode == 58) && (bDown == TRUE)){
		gs_stKeyboardManager.bCapsLockOn ^= TRUE;
		bLEDStatusChanged = TRUE;

	// [69:Num Lock] AND Down Code
	}else if((bDownScanCode == 69) && (bDown == TRUE)){
		gs_stKeyboardManager.bNumLockOn ^= TRUE;
		bLEDStatusChanged = TRUE;

	// [70:Scroll Lock] AND Down Code
	}else if((bDownScanCode == 70) && (bDown == TRUE)){
		gs_stKeyboardManager.bScrollLockOn ^= TRUE;
		bLEDStatusChanged = TRUE;
	}

	// 키보드 LED 상태 변경
	if(bLEDStatusChanged == TRUE){
		kChangeKeyboardLED(gs_stKeyboardManager.bCapsLockOn, gs_stKeyboardManager.bNumLockOn, gs_stKeyboardManager.bScrollLockOn);
	}
}

BOOL kConvertScanCodeToASCIICode(BYTE bScanCode, BYTE* pbASCIICode, BYTE* pbFlags){
	BOOL bUseCombinedKey = FALSE;

	if(gs_stKeyboardManager.iSkipCountForPause > 0){
		gs_stKeyboardManager.iSkipCountForPause--;
		return FALSE;
	}

	// [0xE1:Pause키]인 경우
	if(bScanCode == 0xE1){
		*pbASCIICode = KEY_PAUSE;
		*pbFlags = KEY_FLAGS_DOWN;
		gs_stKeyboardManager.iSkipCountForPause = KEY_SKIPCOUNTFORPAUSE;
		return TRUE;

	// [0xE0:확장키]인 경우
	}else if(bScanCode == 0xE0){
		gs_stKeyboardManager.bExtendedCodeIn = TRUE;
		return FALSE;
	}

	bUseCombinedKey = kIsUseCombinedCode(bScanCode);

	if(bUseCombinedKey == TRUE){
		*pbASCIICode = gs_vstKeyMappingTable[bScanCode & 0x7F].bCombinedCode;

	}else{
		*pbASCIICode = gs_vstKeyMappingTable[bScanCode & 0x7F].bNormalCode;

	}

	if(gs_stKeyboardManager.bExtendedCodeIn == TRUE){
		*pbFlags = KEY_FLAGS_EXTENDEDKEY;
		gs_stKeyboardManager.bExtendedCodeIn = FALSE;

	}else{
		*pbFlags = 0;
	}

	if((bScanCode & 0x80) == 0){
		*pbFlags |= KEY_FLAGS_DOWN;
	}

	kUpdateCombinationKeyStatusAndLED(bScanCode);

	return TRUE;
}

BOOL kInitializeKeyboard(void){
	// 키 큐 초기화
	kInitializeQueue(&gs_stKeyQueue, gs_vstKeyQueueBuffer, KEY_MAXQUEUECOUNT, sizeof(KEYDATA));
	kInitializeSpinLock( &(gs_stKeyboardManager.stSpinLock));
	// 키보드 활성화
	return kActivateKeyboard();
}

BOOL kConvertScanCodeAndPutQueue(BYTE bScanCode){
	KEYDATA stData;
	BOOL bResult = FALSE;
	BOOL bPreviousFlag;

	stData.bScanCode = bScanCode;

	// 스캔 코드->아스키 코드 변환
	if(kConvertScanCodeToASCIICode(bScanCode, &(stData.bASCIICode), &(stData.bFlags)) == TRUE){

		kLockForSpinLock( &(gs_stKeyboardManager.stSpinLock));

		// 키 큐에 데이터 삽입
		bResult = kPutQueue(&gs_stKeyQueue, &stData);

		kUnlockForSpinLock( &(gs_stKeyboardManager.stSpinLock));

	}

	return bResult;
}

BOOL kGetKeyFromKeyQueue(KEYDATA* pstData){
	BOOL bResult = FALSE;
	BOOL bPreviousFlag;

	if(kIsQueueEmpty(&gs_stKeyQueue) == TRUE){
		return FALSE;
	}

	bPreviousFlag = kSetInterruptFlag(FALSE);

	// 키 큐에서 데이터 삭제
	bResult = kGetQueue(&gs_stKeyQueue, pstData);

	kSetInterruptFlag(bPreviousFlag);

	return bResult;
}
