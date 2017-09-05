#ifndef __PIT_H__
#define __PIT_H__

#include "Types.h"

/**** ��ũ�� ���� *****/
// PIT ���� ��ũ��
#define PIT_FREQUENCY 1193182                    // 1.193182 Mhz
#define MSTOCOUNT(x) (PIT_FREQUENCY*(x)/1000)    // MilliSecond to Count
#define USTOCOUNT(x) (PIT_FREQUENCY*(x)/1000000) // MicroSecond to Count

// PIT I/O ��Ʈ
#define PIT_PORT_CONTROL  0x43 // ��Ʈ�� �������� (1byte)
#define PIT_PORT_COUNTER0 0x40 // ī����0 �������� (2byte)
#define PIT_PORT_COUNTER1 0x41 // ī����1 �������� (2byte)
#define PIT_PORT_COUNTER2 0x42 // ī����2 �������� (2byte)

// PIT ��Ʈ�� ��������(1byte)�� �ʵ�
#define PIT_CONTROL_COUNTER0      0x00 // SC(��Ʈ 7,6)=[00:ī����0]
#define PIT_CONTROL_COUNTER1      0x40 // SC(��Ʈ 7,6)=[01:ī����1]
#define PIT_CONTROL_COUNTER2      0x80 // SC(��Ʈ 7,6)=[10:ī����2]
#define PIT_CONTROL_LSBMSBRW      0x30 // RW(��Ʈ 5,4)=[11:ī������ ���� ����Ʈ���� ���� ����Ʈ ������ �����ؼ� I/O��Ʈ�� �аų� ��, 2����Ʈ ����]
#define PIT_CONTROL_LATCH         0x00 // RW(��Ʈ 5,4)=[00:ī������ ���� ���� ����, 2����Ʈ ����(��Ʈ 00���� ������ ���, ��Ʈ 3~0�� ��� 0���� �����ϰ� ���� ����Ʈ���� ���� ����Ʈ ������ �����ؼ� I/O ��Ʈ�� �о�� �Ѵ�.)]
#define PIT_CONTROL_MODE0         0x00 // Mode(��Ʈ 3,2,1)=[000:��� 0(Interrupt during counting, �� ���� ��ȣ �߻�)]
#define PIT_CONTROL_MODE2         0x04 // Mode(��Ʈ 3,2,1)=[010:��� 2(Clock rate generator, �ֱ������� ��ȣ �߻�)]
#define PIT_CONTROL_BINARYCOUNTER 0x00 // BCD(��Ʈ 0)=[0:ī���� ���� ���̳ʸ� �������� ����]
#define PIT_CONTROL_BSDCOUNTER    0x01 // BCD(��Ʈ 1)=[0:ī���� ���� BSD �������� ����]

// ���� ����� ��ũ��
#define PIT_COUNTER0_ONCE     (PIT_CONTROL_COUNTER0 | PIT_CONTROL_LSBMSBRW | PIT_CONTROL_MODE0 | PIT_CONTROL_BINARYCOUNTER)
#define PIT_COUNTER0_PERIODIC (PIT_CONTROL_COUNTER0 | PIT_CONTROL_LSBMSBRW | PIT_CONTROL_MODE2 | PIT_CONTROL_BINARYCOUNTER)
#define PIT_COUNTER0_LATCH    (PIT_CONTROL_COUNTER0 | PIT_CONTROL_LATCH)

/***** �Լ� ���� *****/
void kInitializePIT(WORD wCount, BOOL bPeriodic);
WORD kReadCounter0(void);
void kWaitUsingDirectPIT(WORD wCount);

#endif // __PIT_H__
