#ifndef __SERIALPORT_H__

#define __SERIALPORT_H__

#include "Types.h"
#include "Queue.h"
#include "Synchronization.h"


// 시리얼 포트 컨트롤러의 I/O 포트 기준 어드레스
#define SERIAL_PORT_COM1 0x3F8 // COM1 시리얼 포트 (IRQ4)
#define SERIAL_PORT_COM2 0x2F8 // COM2 시리얼 포트 (IRQ3)
#define SERIAL_PORT_COM3 0x3E8 // COM3 시리얼 포트 (IRQ4)
#define SERIAL_PORT_COM4 0x2E8 // COM4 시리얼 포트 (IRQ3)

// 레지스터(all 1 byte)의 I/O 포트 오프셋
#define SERIAL_PORT_INDEX_RECEIVEBUFFER           0x00 // 수신 버퍼 레지스터(읽기)
#define SERIAL_PORT_INDEX_TRANSMITBUFFER          0x00 // 송신 버퍼 레지스터(쓰기)
#define SERIAL_PORT_INDEX_INTERRUPTENABLE         0x01 // 인터럽트 활성화 레지스터(읽기/쓰기)
#define SERIAL_PORT_INDEX_DIVISORLATCHLSB         0x00 // LSB 제수 래치 레지스터(읽기/쓰기)
#define SERIAL_PORT_INDEX_DIVISORLATCHMSB         0x01 // MSB 제수 래치 레지스터(읽기/쓰기)
#define SERIAL_PORT_INDEX_INTERRUPTIDENTIFICATION 0x02 // 인터럽트 알림 레지스터(읽기)
#define SERIAL_PORT_INDEX_FIFOCONTROL             0x02 // FIFO 제어 레지스터(쓰기)
#define SERIAL_PORT_INDEX_LINECONTROL             0x03 // 라인 제어 레지스터(읽기/쓰기)
#define SERIAL_PORT_INDEX_MODEMCONTROL            0x04 // 모뎀 제어 레지스터(읽기/쓰기)
#define SERIAL_PORT_INDEX_LINESTATUS              0x05 // 라인 상태 레지스터(읽기/쓰기)
#define SERIAL_PORT_INDEX_MODEMSTATUS             0x06 // 모뎀 상태 레지스터(읽기/쓰기)
#define SERIAL_PORT_INDEX_SCRATCHPAD              0x07 // 스크래치 패드 레지스터(읽기/쓰기)

// 인터럽트 활성화 레지스터(1 byte)의 필드
#define SERIAL_INTERRUPTENABLE_RECEIVEBUFFERFULL   0x01 // [비트 0:RxRD] Receive Data Ready, 수신 버퍼 레지스터에 데이터가 찼을 때 인터럽트 발생 여부를 설정
#define SERIAL_INTERRUPTENABLE_TRANSMITBUFFEREMPTY 0x02 // [비트 1:TBE] Transmit Buffer Empty, 송신 버퍼 레지스터가 비었을 때 인터럽트 발생 여부를 설정
#define SERIAL_INTERRUPTENABLE_LINESTATUS          0x04 // [비트 2:EBRK] Error & Break, 데이터를 송수신하는 과정에서 에러가 발생했을 때 인터럽트 발생 여부를 설정
#define SERIAL_INTERRUPTENABLE_DELTASTATUS         0x08 // [비트 3:SINP] Serial Input, 모뎀 상태가 변했을 때 인터럽트 발생 여부를 설정
                                                        // [비트 4~7:None] 사용 안 함

// FIFO 제어 레지스터(1 byte)의 필드
#define SERIAL_FIFOCONTROL_FIFOENABLE        0x01 // [비트 0:FIFO] FIFO 사용 여부를 설정
#define SERIAL_FIFOCONTROL_CLEARRECEIVEFIFO  0x02 // [비트 1:CLRF] Clear Receive FIFO, 수신 FIFO를 비우도록 설정
#define SERIAL_FIFOCONTROL_CLEARTRANSMITFIFO 0x04 // [비트 2:CLTF] Clear Transmit FIFO, 송신 FIFO를 비우도록 설정
#define SERIAL_FIFOCONTROL_ENABLEDMA         0x08 // [비트 3:DMA] DMA 사용 여부를 설정
#define SERIAL_FIFOCONTROL_1BYTEFIFO         0x00 // [비트 6~7:ITL] Interrupt Trigger Level, 인터럽트를 발생시킬 수신 FIFO의 데이터 크기를 설정, 1바이트
#define SERIAL_FIFOCONTROL_4BYTEFIFO         0x40 // [비트 6~7:ITL] Interrupt Trigger Level, 인터럽트를 발생시킬 수신 FIFO의 데이터 크기를 설정, 4바이트
#define SERIAL_FIFOCONTROL_8BYTEFIFO         0x80 // [비트 6~7:ITL] Interrupt Trigger Level, 인터럽트를 발생시킬 수신 FIFO의 데이터 크기를 설정, 8바이트
#define SERIAL_FIFOCONTROL_14BYTEFIFO        0xC0 // [비트 6~7:ITL] Interrupt Trigger Level, 인터럽트를 발생시킬 수신 FIFO의 데이터 크기를 설정, 14바이트
                                                  // [비트 4~5:None] 사용 안 함

// 라인 제어 레지스터(1 byte)의 필드
#define SERIAL_LINECONTROL_5BIT        0x00 // [비트 0~1:D-Bit] Data-Bit, 송수신 프레임안에 저장할 데이터 비트의 길이를 설정, 5비트
#define SERIAL_LINECONTROL_6BIT        0x01 // [비트 0~1:D-Bit] Data-Bit, 송수신 프레임안에 저장할 데이터 비트의 길이를 설정, 6비트
#define SERIAL_LINECONTROL_7BIT        0x02 // [비트 0~1:D-Bit] Data-Bit, 송수신 프레임안에 저장할 데이터 비트의 길이를 설정, 7비트
#define SERIAL_LINECONTROL_8BIT        0x03 // [비트 0~1:D-Bit] Data-Bit, 송수신 프레임안에 저장할 데이터 비트의 길이를 설정, 8비트(일반적으로 8비트 사용)
#define SERIAL_LINECONTROL_1BITSTOP    0x00 // [비트 2:STOP] 정지 비트의 길이를 설정, 1비트(일반적으로 1비트 사용)
#define SERIAL_LINECONTROL_2BITSTOP    0x04 // [비트 2:STOP] 정지 비트의 길이를 설정, 2비트 또는 1.5비트
#define SERIAL_LINECONTROL_NOPARITY    0x00 // [비트 3~5:Parity] 패리티 사용 여부를 설정, 패리티 사용 안 함
#define SERIAL_LINECONTROL_ODDPARITY   0x08 // [비트 3~5:Parity] 패리티 사용 여부를 설정, 홀수 패리티 사용
#define SERIAL_LINECONTROL_EVENPARITY  0x18 // [비트 3~5:Parity] 패리티 사용 여부를 설정, 짝수 패리티 사용
#define SERIAL_LINECONTROL_MARKPARITY  0x28 // [비트 3~5:Parity] 패리티 사용 여부를 설정, 패리티 비트를 항상 1로 설정
#define SERIAL_LINECONTROL_SPACEPARITY 0x38 // [비트 3~5:Parity] 패리티 사용 여부를 설정, 패리티 비트를 항상 0으로 설정
#define SERIAL_LINECONTROL_BREAK       0x40 // [비트 6:BRK] Break, 데이터 송수신을 중단하는 Break 신호를 전송할지 여부를 설정
#define SERIAL_LINECONTROL_DLAB        0x80 // [비트 7:DLAB] Divisor Latch Access Bit, 제수 래치 레지스터에 접근할지 여부를 설정

// 라인 상태 레지스터(1 byte)의 필드
#define SERIAL_LINESTATUS_RECEIVEDDATAREADY      0x01 // [비트 0:RxRD] Receive Data Ready, 수신 버퍼 레지스터에 데이터가 수신되었음을 의미
#define SERIAL_LINESTATUS_OVERRUNERROR           0x02 // [비트 1:OVRE] Overrun Error, 수신 버퍼 레지스터가 가득 차서 더 이상 데이터를 수신할 수 없음을 의미
#define SERIAL_LINESTATUS_PARITYERROR            0x04 // [비트 2:PARE] Parity Error, 수신된 데이터에 패리티 에러가 발생했음을 의미
#define SERIAL_LINESTATUS_FRAMINGERROR           0x08 // [비트 3:FRME] Frame Error, 수신된 데이터에 프레임 에러가 발생했음을 의미
#define SERIAL_LINESTATUS_BREAKINDICATOR         0x10 // [비트 4:BREK] Break, Break 신호가 검출되었음을 의미
#define SERIAL_LINESTATUS_TRANSMITBUFFEREMPTY    0x20 // [비트 5:TBE] Transmit Buffer Empty, 송신 버퍼 레지스터가 비었음을 의미
#define SERIAL_LINESTATUS_TRANSMITEMPTY          0x40 // [비트 6:TXE] Transmit Empty, 현재 송신중인 데이터가 없음을 의미
#define SERIAL_LINESTATUS_RECEIVEDCHARACTERERROR 0x80 // [비트 7:FIFOE] FIFO Error, 수신 FIFO에 저장된 데이터 중에 에러가 발생한 데이터가 있음을 의미

// 제수 래치 레지스터(MSB=1 byte, LSB=1 byte)의 필드
#define SERIAL_DIVISORLATCH_115200 1  // [MSB=0, LSB=1]  시리얼 통신 속도 설정, 115200 Bd or Bps, 1초당 약 14KB를 전송, MINT64에서 사용
#define SERIAL_DIVISORLATCH_57600  2  // [MSB=0, LSB=2]  시리얼 통신 속도 설정,  57600 Bd or Bps
#define SERIAL_DIVISORLATCH_38400  3  // [MSB=0, LSB=3]  시리얼 통신 속도 설정,  38400 Bd or Bps
#define SERIAL_DIVISORLATCH_19200  6  // [MSB=0, LSB=6]  시리얼 통신 속도 설정,  19200 Bd or Bps
#define SERIAL_DIVISORLATCH_9600   12 // [MSB=0, LSB=12] 시리얼 통신 속도 설정,   9600 Bd or Bps
#define SERIAL_DIVISORLATCH_4800   24 // [MSB=0, LSB=24] 시리얼 통신 속도 설정,   4800 Bd or Bps
#define SERIAL_DIVISORLATCH_2400   48 // [MSB=0, LSB=48] 시리얼 통신 속도 설정,   2400 Bd or Bps [참고: Bd = Baud Rate, Bps = Bit Per Second]

// 기타 매크로
#define SERIAL_FIFOMAXSIZE 16 // FIFO 최대 크기(16 byte)

/***** 구조체 정의 *****/
#pragma pack(push, 1)

typedef struct kSerialPortManager{
	MUTEX stMutex; // 뮤텍스 동기화 객체
} SERIALMANAGER;

#pragma pack(pop)

void kInitializeSerialPort(void);
static BOOL kIsSerialTransmitterBufferEmpty(void);
void kSendSerialData(BYTE* pbBuffer, int iSize);
static BOOL kIsSerialRecevieBufferFull(void);
int kReceivesSerialData(BYTE* pbBuffer, int iSize);
void kClearSerialFIFO(void);

#endif