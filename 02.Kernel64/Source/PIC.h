#ifndef __PIC_H__
#define __PIC_H__

#include "Types.h"

// 매크로 정의
// I/O 포트 정의
#define PIC_MASTER_PORT1 0x20
#define PIC_MASTER_PORT2 0x21
#define PIC_SLAVE_PORT1  0xA0
#define PIC_SLAVE_PORT2  0xA1

// IDT에서 인터럽트 벡터의 시작 위치
// IRQ0~15 -> 벡터 32(0x20)~47(0x2F)
#define PIC_IRQSTARTVECTOR 0x20

// 함수 정의
void kInitializePIC(void);
void kMaskPICInterrupt(WORD wIRQBitmask);
void kSendEOIToPIC(int iIRQNumber);

#endif // __PIC_H__
