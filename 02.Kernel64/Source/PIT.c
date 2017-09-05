#include "PIT.h"
#include "AssemblyUtility.h"

void kInitializePIT(WORD wCount, BOOL bPeriodic){

	// ��Ʈ�� �������Ϳ� Once Ŀ�ǵ� ����
	kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_ONCE);

	if(bPeriodic == TRUE){
		// ��Ʈ�� �������Ϳ� Periodic Ŀ�ǵ� ����
		kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_PERIODIC);
	}

	// ī���� 0�� LSB->MSB������ ī���� �ʱⰪ�� ��
	kOutPortByte(PIT_PORT_COUNTER0, wCount);
	kOutPortByte(PIT_PORT_COUNTER0, wCount >> 8);
}

WORD kReadCounter0(void){
	BYTE bHighByte, bLowByte;
	WORD wTemp = 0;

	// ��Ʈ�� �������Ϳ� Latch Ŀ�ǵ带 �����Ͽ� ī���� 0�� ���� ���� ����
	kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_LATCH);

	// ī���� 0���� LSB->MSB������ ī���� ���� ���� ����
	bLowByte = kInPortByte(PIT_PORT_COUNTER0);
	bHighByte = kInPortByte(PIT_PORT_COUNTER0);

	wTemp = bHighByte;
	wTemp = (wTemp << 8) | bLowByte;

	return wTemp;
}

/* ī���� 0�� ���� �����Ͽ� ���� �ð� �̻� ���
 * -�Ʒ� �Լ��� ȣ���ϸ� PIT ��Ʈ�ѷ��� ������ �ٲ�Ƿ�, ���Ŀ� PIT ��Ʈ�ѷ��� �缳���ؾ� ��
 * -��Ȯ�ϰ� �����Ϸ��� �Լ� ȣ�� ���� ���ͷ�Ʈ�� ��Ȱ��ȭ�ϴ� ���� ����
 * -54.93ms���� ���� ����
 */
void kWaitUsingDirectPIT(WORD wCount){
	WORD wLastCounter0;
	WORD wCurrentCounter0;

	// PIT ��Ʈ�ѷ��� 0~0xFFFF���� �ݺ��ؼ� ī�����ϵ��� ����
	kInitializePIT(0, TRUE);

	// ���ݺ��� wCount�� �̻� ������ ������ ���
	wLastCounter0 = kReadCounter0();
	while(1){
		wCurrentCounter0 = kReadCounter0();
		if(((wCurrentCounter0 - wLastCounter0) & 0xFFFF) >= wCount){
			break;
		}
	}
}
