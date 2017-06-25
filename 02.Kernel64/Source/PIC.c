#include "PIC.h"

void kInitializePIC(void){
	/***** 마스터 PIC 초기화 *****/
	// 포트 0x20, 커맨드 ICW1: 0x11 -> 0001 0001 -> LTIM(비트 3)=0, SNGL(비트 1)=0, IC4(비트 0)=1
	kOutPortByte(PIC_MASTER_PORT1, 0x11);

	// 포트 0x21, 커맨드 ICW2: 0x20 -> 0010 0000 -> IDT에서 마스터 인터럽트 벡터의 시작 위치(32)
	kOutPortByte(PIC_MASTER_PORT2, PIC_IRQSTARTVECTOR);

	// 포트 0x21, 커맨드 ICW3: 0x04 -> 0000 0100 -> 마스터 PIC에서 슬레이브 PIC가 연결된 핀의 위치(비트 오프셋 사용)
	kOutPortByte(PIC_MASTER_PORT2, 0x04);

	// 포트 0x21, 커맨드 ICW4: 0x01 -> 0000 0001 -> SFNM(비트 4)=0, BUF(비트 3)=0, M/S(비트 2)=0, AEOI(비트 1)=0, uPM(비트 0)=1
	kOutPortByte(PIC_MASTER_PORT2, 0x01);

	/***** 슬레이브 PIC 초기화 *****/
	// 포트 0xA0, 커맨드 ICW1: 0x11 -> 0001 0001 -> LTIM(비트 3)=0, SNGL(비트 1)=0, IC4(비트 0)=1
	kOutPortByte(PIC_SLAVE_PORT1, 0x11);

	// 포트 0xA1, 커맨드 ICW2: 0x28 -> 0010 1000 -> IDT에서 슬레이브 인터럽트 벡터의 시작 위치(40)
	kOutPortByte(PIC_SLAVE_PORT2, PIC_IRQSTARTVECTOR + 8);

	// 포트 0xA1, 커맨드 ICW3: 0x02 -> 0000 0010 -> 마스터 PIC에서 슬레이브 PIC가 연결된 핀의 위치(정수 값 사용)
	kOutPortByte(PIC_SLAVE_PORT2, 0x02);

	// 포트 0xA1, 커맨드 ICW4: 0x01 -> 0000 0001 -> SFNM(비트 4)=0, BUF(비트 3)=0, M/S(비트 2)=0, AEOI(비트 1)=0, uPM(비트 0)=1
	kOutPortByte(PIC_SLAVE_PORT2, 0x01);
}

void kMaskPICInterrupt(WORD wIRQBitmask){
	/***** 마스터 PIC 마스크 *****/
	// 포트 0x21, 커맨드 OCW1: 0x?? -> ???? ???? -> 해당 핀의 인터럽트를 마스크함
	kOutPortByte(PIC_MASTER_PORT2, (BYTE)wIRQBitmask);

	/***** 슬레이브 PIC 마스크 *****/
	// 포트 0xA1, 커맨드 OCW1: 0x?? -> ???? ???? -> 해당 핀의 인터럽트를 마스크함
	kOutPortByte(PIC_SLAVE_PORT2, (BYTE)(wIRQBitmask >> 8));
}

void kSendEOIToPIC(int iIRQNumber){
	/* 마스터 PIC에 인터럽트가 발생한 경우, 마스터 PIC에만 EOI 전송
	 * 슬레이브 PIC에 인터럽트가 발생한 경우, 마스터 PIC와 슬레이브 PIC 모두에게 EOI 전송
	 */

	/***** 마스터 PIC에 EOI 전송 *****/
	// 포트 0x20, 커맨드 OCW2: 0x20 -> 0010 0000 -> R(비트 7)=0, SL(비트 6)=0, EOI(비트 5)=1, L2(비트 2)=0, L1(비트 1)=0, L0(비트 0)=0
	kOutPortByte(PIC_MASTER_PORT1, 0x20);

	/***** 슬레이브 PIC에 EOI 전송 *****/
	if(iIRQNumber >= 8){
		// 포트 0xA0, 커맨드 OCW2: 0x20 -> 0010 0000 -> R(비트 7)=0, SL(비트 6)=0, EOI(비트 5)=1, L2(비트 2)=0, L1(비트 1)=0, L0(비트 0)=0
		kOutPortByte(PIC_SLAVE_PORT1, 0x20);
	}
}

