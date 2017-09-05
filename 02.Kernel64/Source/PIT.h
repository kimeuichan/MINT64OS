#ifndef __PIT_H__
#define __PIT_H__

#include "Types.h"

/**** 매크로 정의 *****/
// PIT 관련 매크로
#define PIT_FREQUENCY 1193182                    // 1.193182 Mhz
#define MSTOCOUNT(x) (PIT_FREQUENCY*(x)/1000)    // MilliSecond to Count
#define USTOCOUNT(x) (PIT_FREQUENCY*(x)/1000000) // MicroSecond to Count

// PIT I/O 포트
#define PIT_PORT_CONTROL  0x43 // 컨트롤 레지스터 (1byte)
#define PIT_PORT_COUNTER0 0x40 // 카운터0 레지스터 (2byte)
#define PIT_PORT_COUNTER1 0x41 // 카운터1 레지스터 (2byte)
#define PIT_PORT_COUNTER2 0x42 // 카운터2 레지스터 (2byte)

// PIT 컨트롤 레지스터(1byte)의 필드
#define PIT_CONTROL_COUNTER0      0x00 // SC(비트 7,6)=[00:카운터0]
#define PIT_CONTROL_COUNTER1      0x40 // SC(비트 7,6)=[01:카운터1]
#define PIT_CONTROL_COUNTER2      0x80 // SC(비트 7,6)=[10:카운터2]
#define PIT_CONTROL_LSBMSBRW      0x30 // RW(비트 5,4)=[11:카운터의 하위 바이트에서 상위 바이트 순으로 연속해서 I/O포트를 읽거나 씀, 2바이트 전송]
#define PIT_CONTROL_LATCH         0x00 // RW(비트 5,4)=[00:카운터의 현재 값을 읽음, 2바이트 전송(비트 00으로 설정할 경우, 비트 3~0은 모두 0으로 설정하고 하위 바이트에서 상위 바이트 순으로 연속해서 I/O 포트를 읽어야 한다.)]
#define PIT_CONTROL_MODE0         0x00 // Mode(비트 3,2,1)=[000:모드 0(Interrupt during counting, 한 번만 신호 발생)]
#define PIT_CONTROL_MODE2         0x04 // Mode(비트 3,2,1)=[010:모드 2(Clock rate generator, 주기적으로 신호 발생)]
#define PIT_CONTROL_BINARYCOUNTER 0x00 // BCD(비트 0)=[0:카운터 값을 바이너리 포맷으로 설정]
#define PIT_CONTROL_BSDCOUNTER    0x01 // BCD(비트 1)=[0:카운터 값을 BSD 포맷으로 설정]

// 자주 사용할 매크로
#define PIT_COUNTER0_ONCE     (PIT_CONTROL_COUNTER0 | PIT_CONTROL_LSBMSBRW | PIT_CONTROL_MODE0 | PIT_CONTROL_BINARYCOUNTER)
#define PIT_COUNTER0_PERIODIC (PIT_CONTROL_COUNTER0 | PIT_CONTROL_LSBMSBRW | PIT_CONTROL_MODE2 | PIT_CONTROL_BINARYCOUNTER)
#define PIT_COUNTER0_LATCH    (PIT_CONTROL_COUNTER0 | PIT_CONTROL_LATCH)

/***** 함수 선언 *****/
void kInitializePIT(WORD wCount, BOOL bPeriodic);
WORD kReadCounter0(void);
void kWaitUsingDirectPIT(WORD wCount);

#endif // __PIT_H__
