#ifndef __VBE_H__

#define __VBE_H__

#include "Types.h"


// 매크로
// 모드 정보 블록이 저장된 어드레스
#define VBE_MODEINFOBLOCKADDRESS	0x7e00

// 그래픽 모드로 시작하는 플래그가 저장된 어드레스
#define VBE_STARTGRAPHICMODEFLAGADDRESS	0x7c0a



#pragma pack(push, 1)
// VBE에서 정의한 모드 정보 블록 자료구조, 256 바이트
typedef struct kVBEInfoBlockStruct{
	// ===================
	// 모든 VBE 버전 공통 부분
	// ===================
	// 모드의 속성
	WORD wModeAttribute;
	// 윈도우 A의 속성
	BYTE bWinAAttribute;
	// 윈도우 B의 속성
	BYTE bWinBAttribute;
	// 윈도우의 가중치
	WORD wWinGranulity;
	// 윈도우의 크기
	WORD wWinSize;
	// 윈도우 A가 시작하는 세그먼트 어드레스
	WORD wWinASegment;
	// 윈도우 B가 시작하는 세그먼트 어드레스
	WORD wWinBSegment;
	// 윈도우 관련 함수의 포인터(리얼 모드용)
	DWORD dwWinFuncPtr;
	// 화면 스캔 라인 당 바이트 수
	WORD wBytesPerScanLine;

	// ===================
	// VBE 1.2 이상 공통 부분
	// ===================
	// X축 픽셀 수 또는 문자 수
	WORD wXResolution;
	// Y축 픽셀 수 또는 문자 수
	WORD wYResolution;
	// 한 문자의 X축 픽셀 수
	BYTE bXCharSize;
	// 한 문자의 Y축 픽셀 수
	BYTE bYCharSize;
	// 메모리 플레인 수
	BYTE bNumberOfPlane;
	// 한 픽셀을 구성하는 비트 수
	BYTE bBitsPerPixel;
	// 뱅크 수
	BYTE bNumberOfBanks;
	// 비디오 메모리 구성
	BYTE bMemoryModel;
	// 뱅크의 크기(KB)
	BYTE bBankSize;
	// 이미지 페이지 개수
	BYTE bNumberOfImagePages;
	// 페이지 기능을 위해 예약된 영역
	BYTE bReserved;

	// 다이렉트 컬러(Direct Color)에 관련된 필드
	// 빨간색 필드가 차지하는 크기
	BYTE bRedMaskSize;
	// 빨간색 필드의 위치
	BYTE bRedFieldPosition;
	// 녹색 필드가 차지하는 크기
	BYTE bGreenMaskSize;
	// 녹색 필드의 위치
	BYTE bGreenFieldPosition;
	// 파란색 필드가 차지하는 크기
	BYTE bBlueMaskSize;
	// 파란색 필드의 위치
	BYTE bBlueFieldPosition;
	// 예약된 필드의 크기
	BYTE bReservedMaskSize;
	// 예약된 필드의 위치
	BYTE bReservedFieldPosition;
	// 다이렉트 컬러 모드의 저보
	BYTE bDirectColorModeInfo;

	// ===================
	// VBE 2.0 이상 공통 부분
	// =================== 
	// 선형 프레임 버퍼 메모리 시작 어드레스
	DWORD dwPhysicalBasePointer;
	// 예약된 필드
	DWORD dwReserved1;
	DWORD dwReserved2;

	// ===================
	// VBE 3.0 이상 공통 부분
	// =================== 
	// 선형 프레임 버퍼 모드의 화면 스캔 라인당 바이트 수
	WORD wLinearBytesPerScanLine;
	// 뱅크 모드일 때 이미지 페이지 수
	BYTE bBankNumberOfImagePages;
	// 선형 프레임 버퍼 모드일 때 이미지 페이지 수
	BYTE bLinearNumberOfImagePages;
	// 선형 프레임 버퍼 모드일 때 다이렉트 컬러에 관련된 필드
	// 빨간색 필드가 차지하는 크기
	BYTE bLinearRedMaskSize;
	// 빨간색 필드의 위치
	BYTE bLinearRedFieldPosition;
	// 녹색 필드가 차지하는 크기
	BYTE bLinearGreenMaskSize;
	// 녹색 필드의 위치
	BYTE bLinearGreenFieldPosition;
	// 파란색 필드가 차지하는 크기
	BYTE bLinearBlueMaskSize;
	// 파란색 필드의 위치
	BYTE bLinearBlueFieldPosition;
	// 예약된 필드의 크기
	BYTE bLinearReservedMaskSize;
	// 예약된 필드의 위치
	BYTE bLinearReservedFieldPosition;
	// 픽셀 클록의 최댓값(Hz)
	DWORD dwMaxPixelClock;

	// 나머지 영역
	BYTE vbReserved[189];
} VBEMODEINFOBLOCK;
#pragma pack(pop)


// 함수
VBEMODEINFOBLOCK* kGetVBEModeInfoBlock(void);

#endif
