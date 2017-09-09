#include "SerialPort.h"
#include "Utility.h"

static SERIALMANAGER gs_stSerialManager;

void kInitializeSerialPort(void){
	WORD wPortBase;

	kInitializeMutex( &(gs_stSerialManager.stMutex));

	wPortBase = SERIAL_PORT_COM1;

	// 인터럽트 비활성화
	kOutPortByte(wPortBase+SERIAL_PORT_INDEX_INTERRUPTENABLE, 0);

	// 통신 속도를 115200 으로 설정
	// 라인 제어 레지스터의 DLAB 비트를 1로 설정하여 제수 래치 레지스터에 접근
	kOutPortByte(wPortBase+SERIAL_PORT_INDEX_LINECONTROL, SERIAL_LINECONTROL_DLAB);

	// lsb 제수 래치 레지스터에 재수의 하위 8비트를 전송
	kOutPortByte(wPortBase+SERIAL_PORT_INDEX_DIVISORLATCHLSB, SERIAL_DIVISORLATCH_115200);
	// msb 제수 래치 레지스터에 재수의 하위 8비트를 전송
	kOutPortByte(wPortBase+SERIAL_PORT_INDEX_DIVISORLATCHMSB, SERIAL_DIVISORLATCH_115200 >> 8);

	// 송수신 방법을 설정
	// 라인 제어 레지스터에 통신 방법을 8비트, 패러티 사용x 설정
	// 1 stop 비트로 설정하고, 제수 래치 레지스터 사용이 끝났으므로 dlab 비트를 0으로 설정
	kOutPortByte(wPortBase+SERIAL_PORT_INDEX_LINECONTROL, SERIAL_LINECONTROL_8BIT | SERIAL_LINECONTROL_NOPARITY | SERIAL_LINECONTROL_1BITSTOP);

	// fifo 인터럽트 발생 시점을 14바이트로 설정
	kOutPortByte(wPortBase+SERIAL_PORT_INDEX_FIFOCONTROL, SERIAL_LINECONTROL_8BIT | SERIAL_FIFOCONTROL_FIFOENABLE | SERIAL_FIFOCONTROL_14BYTEFIFO);
}

static BOOL kIsSerialTransmitterBufferEmpty(void){
	BYTE bData;

	// 라인 상태 레지스터 읽은 뒤 TBE 비트 확인하여
	// 송신 fifo 비어 잇는지 확인
	bData = kInPortByte(SERIAL_PORT_COM1+SERIAL_PORT_INDEX_LINESTATUS);
	if( (bData & SERIAL_LINESTATUS_TRANSMITBUFFEREMPTY) == SERIAL_LINESTATUS_TRANSMITBUFFEREMPTY)
		return TRUE;
	return FALSE;
}

void kSendSerialData(BYTE* pbBuffer, int iSize){
	// 보낸 양 저장
	int iSentByte;
	// 한 번 보낼 때 임시 사이즈 저장
	int iTempSize;
	int i;

	kLock( &(gs_stSerialManager.stMutex));

	// 요청한 바이트 수만큼 보낼때까지 반복
	iSentByte = 0;
	while(iSentByte < iSize){
		// 송신 FIFO에 데이터가 남아있다면 다 전송될때까지 대기
		while(kIsSerialTransmitterBufferEmpty() == FALSE)
			kSleep(0);

		// 전송할 데이터 중에서 남은 크기와 fifo 최대 크기비교후 작은것을 선택하여 송신 시리얼 포트를 채움
		iTempSize = MIN(iSize - iSentByte, SERIAL_FIFOMAXSIZE);
		for(i=0; i<iTempSize; i++)
			kOutPortByte(SERIAL_PORT_COM1+SERIAL_PORT_INDEX_TRANSMITBUFFER, pbBuffer[iSentByte+i]);
		iSentByte += iTempSize;
	}

	kUnlock( &(gs_stSerialManager.stMutex));
}

static BOOL kIsSerialRecevieBufferFull(void){
	BYTE bData;

	bData = kInPortByte(SERIAL_PORT_COM1+SERIAL_PORT_INDEX_LINESTATUS);
	if( (bData & SERIAL_LINESTATUS_RECEIVEDDATAREADY) == SERIAL_LINESTATUS_RECEIVEDDATAREADY)
		return TRUE;

	return FALSE;
}

int kReceivesSerialData(BYTE* pbBuffer, int iSize){
	int i;

	kLock( &(gs_stSerialManager.stMutex));

	for(i=0; i<iSize; i++){
		if(kIsSerialRecevieBufferFull() == FALSE)
			break;
		pbBuffer[i] = kInPortByte(SERIAL_PORT_COM1+SERIAL_PORT_INDEX_RECEIVEBUFFER);
	}

	kUnlock( &(gs_stSerialManager.stMutex));
	return i;
}

void kClearSerialFIFO(void){
	kLock( &(gs_stSerialManager.stMutex));

	kOutPortByte(SERIAL_PORT_COM1+SERIAL_PORT_INDEX_FIFOCONTROL, SERIAL_FIFOCONTROL_FIFOENABLE | SERIAL_FIFOCONTROL_14BYTEFIFO | SERIAL_FIFOCONTROL_CLEARRECEIVEFIFO | SERIAL_FIFOCONTROL_CLEARTRANSMITFIFO);

	kUnlock( &(gs_stSerialManager.stMutex));
}