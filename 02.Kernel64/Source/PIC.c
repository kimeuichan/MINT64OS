#include "PIC.h"

void kInitializePIC(void){
	/***** ������ PIC �ʱ�ȭ *****/
	// ��Ʈ 0x20, Ŀ�ǵ� ICW1: 0x11 -> 0001 0001 -> LTIM(��Ʈ 3)=0, SNGL(��Ʈ 1)=0, IC4(��Ʈ 0)=1
	kOutPortByte(PIC_MASTER_PORT1, 0x11);

	// ��Ʈ 0x21, Ŀ�ǵ� ICW2: 0x20 -> 0010 0000 -> IDT���� ������ ���ͷ�Ʈ ������ ���� ��ġ(32)
	kOutPortByte(PIC_MASTER_PORT2, PIC_IRQSTARTVECTOR);

	// ��Ʈ 0x21, Ŀ�ǵ� ICW3: 0x04 -> 0000 0100 -> ������ PIC���� �����̺� PIC�� ����� ���� ��ġ(��Ʈ ������ ���)
	kOutPortByte(PIC_MASTER_PORT2, 0x04);

	// ��Ʈ 0x21, Ŀ�ǵ� ICW4: 0x01 -> 0000 0001 -> SFNM(��Ʈ 4)=0, BUF(��Ʈ 3)=0, M/S(��Ʈ 2)=0, AEOI(��Ʈ 1)=0, uPM(��Ʈ 0)=1
	kOutPortByte(PIC_MASTER_PORT2, 0x01);

	/***** �����̺� PIC �ʱ�ȭ *****/
	// ��Ʈ 0xA0, Ŀ�ǵ� ICW1: 0x11 -> 0001 0001 -> LTIM(��Ʈ 3)=0, SNGL(��Ʈ 1)=0, IC4(��Ʈ 0)=1
	kOutPortByte(PIC_SLAVE_PORT1, 0x11);

	// ��Ʈ 0xA1, Ŀ�ǵ� ICW2: 0x28 -> 0010 1000 -> IDT���� �����̺� ���ͷ�Ʈ ������ ���� ��ġ(40)
	kOutPortByte(PIC_SLAVE_PORT2, PIC_IRQSTARTVECTOR + 8);

	// ��Ʈ 0xA1, Ŀ�ǵ� ICW3: 0x02 -> 0000 0010 -> ������ PIC���� �����̺� PIC�� ����� ���� ��ġ(���� �� ���)
	kOutPortByte(PIC_SLAVE_PORT2, 0x02);

	// ��Ʈ 0xA1, Ŀ�ǵ� ICW4: 0x01 -> 0000 0001 -> SFNM(��Ʈ 4)=0, BUF(��Ʈ 3)=0, M/S(��Ʈ 2)=0, AEOI(��Ʈ 1)=0, uPM(��Ʈ 0)=1
	kOutPortByte(PIC_SLAVE_PORT2, 0x01);
}

void kMaskPICInterrupt(WORD wIRQBitmask){
	/***** ������ PIC ����ũ *****/
	// ��Ʈ 0x21, Ŀ�ǵ� OCW1: 0x?? -> ???? ???? -> �ش� ���� ���ͷ�Ʈ�� ����ũ��
	kOutPortByte(PIC_MASTER_PORT2, (BYTE)wIRQBitmask);

	/***** �����̺� PIC ����ũ *****/
	// ��Ʈ 0xA1, Ŀ�ǵ� OCW1: 0x?? -> ???? ???? -> �ش� ���� ���ͷ�Ʈ�� ����ũ��
	kOutPortByte(PIC_SLAVE_PORT2, (BYTE)(wIRQBitmask >> 8));
}

void kSendEOIToPIC(int iIRQNumber){
	/* ������ PIC�� ���ͷ�Ʈ�� �߻��� ���, ������ PIC���� EOI ����
	 * �����̺� PIC�� ���ͷ�Ʈ�� �߻��� ���, ������ PIC�� �����̺� PIC ��ο��� EOI ����
	 */

	/***** ������ PIC�� EOI ���� *****/
	// ��Ʈ 0x20, Ŀ�ǵ� OCW2: 0x20 -> 0010 0000 -> R(��Ʈ 7)=0, SL(��Ʈ 6)=0, EOI(��Ʈ 5)=1, L2(��Ʈ 2)=0, L1(��Ʈ 1)=0, L0(��Ʈ 0)=0
	kOutPortByte(PIC_MASTER_PORT1, 0x20);

	/***** �����̺� PIC�� EOI ���� *****/
	if(iIRQNumber >= 8){
		// ��Ʈ 0xA0, Ŀ�ǵ� OCW2: 0x20 -> 0010 0000 -> R(��Ʈ 7)=0, SL(��Ʈ 6)=0, EOI(��Ʈ 5)=1, L2(��Ʈ 2)=0, L1(��Ʈ 1)=0, L0(��Ʈ 0)=0
		kOutPortByte(PIC_SLAVE_PORT1, 0x20);
	}
}

