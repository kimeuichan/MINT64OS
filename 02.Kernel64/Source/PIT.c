#include "PIT.h"
#include "AssemblyUtility.h"

void kInitializePIT(WORD wCount, BOOL bPeriodic){

	// 컨트롤 레지스터에 Once 커맨드 전송
	kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_ONCE);

	if(bPeriodic == TRUE){
		// 컨트롤 레지스터에 Periodic 커맨드 전송
		kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_PERIODIC);
	}

	// 카운터 0에 LSB->MSB순으로 카운터 초기값을 씀
	kOutPortByte(PIT_PORT_COUNTER0, wCount);
	kOutPortByte(PIT_PORT_COUNTER0, wCount >> 8);
}

WORD kReadCounter0(void){
	BYTE bHighByte, bLowByte;
	WORD wTemp = 0;

	// 컨트롤 레지스터에 Latch 커맨드를 전송하여 카운터 0의 현재 값을 읽음
	kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_LATCH);

	// 카운터 0에서 LSB->MSB순으로 카운터 현재 값을 읽음
	bLowByte = kInPortByte(PIT_PORT_COUNTER0);
	bHighByte = kInPortByte(PIT_PORT_COUNTER0);

	wTemp = bHighByte;
	wTemp = (wTemp << 8) | bLowByte;

	return wTemp;
}

/* 카운터 0을 직접 설정하여 일정 시간 이상 대기
 * -아래 함수를 호출하면 PIT 컨트롤러의 설정이 바뀌므로, 이후에 PIT 컨트롤러를 재설정해야 함
 * -정확하게 측정하려면 함수 호출 전에 인터럽트를 비활성화하는 것이 좋음
 * -54.93ms까지 측정 가능
 */
void kWaitUsingDirectPIT(WORD wCount){
	WORD wLastCounter0;
	WORD wCurrentCounter0;

	// PIT 컨트롤러를 0~0xFFFF까지 반복해서 카운팅하도록 설정
	kInitializePIT(0, TRUE);

	// 지금부터 wCount값 이상 증가할 때까지 대기
	wLastCounter0 = kReadCounter0();
	while(1){
		wCurrentCounter0 = kReadCounter0();
		if(((wCurrentCounter0 - wLastCounter0) & 0xFFFF) >= wCount){
			break;
		}
	}
}
