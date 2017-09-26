
#ifndef __LOCALAPIC_H__

#define __LOCALAPIC_H__

#include "Types.h"

// 매크로

// 로컬 APIC 레지스터 오프셋 관련 매지스터
#define APIC_REGISTER_EOI	0x0000b0
#define APIC_REGISTER_SVR	0x0000f0
#define APIC_REGISTER_APICID	0x000020
#define APIC_REGISTER_TASKPRIORITY	0x000080
#define APIC_REGISTER_TIMER	0x000320
#define APIC_REGISTER_THERMALSENSOR	0x000330
#define APIC_REGISTER_PERFORMANCEMOITORINGCOUNTER	0x000340
#define APIC_REGISTER_LINT0	0x000350
#define APIC_REGISTER_LINT1	0x000360
#define APIC_REGISTER_ERROR	0x000370
#define APIC_REGISTER_ICR_LOWER	0x000300
#define APIC_REGISTER_ICR_UPPER	0x000310

// 의사 인터럽트 벡터 레지스터(32비트) - 로컬 APIC 소프트웨어 활성/비활성(비트 8)
#define APIC_SOFTWARE_DISABLE 0x000 // 0 : 로컬 APIC 비활성화
#define APIC_SOFTWARE_ENABLE  0x100 // 1 : 로컬 APIC 활성화

// 인터럽트 커맨드 레지스터(64 비트) - 전달 모드(비트 8~10)
#define APIC_DELIVERYMODE_FIXED          0x000000 // 000 : 고정
#define APIC_DELIVERYMODE_LOWESTPRIORITY 0x000100 // 001 : 최하위 우선순위
#define APIC_DELIVERYMODE_SMI            0x000200 // 010 : 시스템 관리 인터럽트
#define APIC_DELIVERYMODE_RESERVED       0x000300 // 011 : 예약됨
#define APIC_DELIVERYMODE_NMI            0x000400 // 100 : 마스크할 수 없는 인터럽트
#define APIC_DELIVERYMODE_INIT           0x000500 // 101 : 초기화
#define APIC_DELIVERYMODE_STARTUP        0x000600 // 110 : 시작
#define APIC_DELIVERYMODE_EXTINT         0x000700 // 111 : 예약됨

// 인터럽트 커맨드 레지스터(64 비트) - 목적지 모드(비트 11)
#define APIC_DESTINATIONMODE_PHYSICAL 0x000000 // 0 : 물리 목적지
#define APIC_DESTINATIONMODE_LOGICAL  0x000800 // 1 : 논리 목적지

// 인터럽트 커맨드 레지스터(64 비트) - 전달 상태(비트 12)
#define APIC_DELIVERYSTATUS_IDLE    0x000000 // 0 : 유휴 상태
#define APIC_DELIVERYSTATUS_PENDING 0x001000 // 1 : 전송 대기중 상태

// 인터럽트 커맨드 레지스터(64 비트) - 레벨(비트 14)
#define APIC_LEVEL_DEASSERT 0x000000 // 0 : 디어설트
#define APIC_LEVEL_ASSERT   0x004000 // 1 : 어설트

// 인터럽트 커맨드 레지스터(64 비트) - 트리거 모드(비트 15)
#define APIC_TRIGGERMODE_EDGE  0x000000 // 0 : 엣지 트리거
#define APIC_TRIGGERMODE_LEVEL 0x008000 // 1 : 레벨 트리거

// 인터럽트 커맨드 레지스터(64 비트) - 목적지 약어(비트 18~19)
#define APIC_DESTINATIONSHORTHAND_NOSHORTHAND      0x000000 // 00 : 약어 사용 안 함
#define APIC_DESTINATIONSHORTHAND_SELF             0x040000 // 01 : 자신
#define APIC_DESTINATIONSHORTHAND_ALLINCLUDINGSELF 0x080000 // 10 : 자신을 포함한 모든 로컬 APIC
#define APIC_DESTINATIONSHORTHAND_ALLEXCLUDINGSELF 0x0C0000 // 11 : 자신을 제외한 모든 로컬 APIC

// 인터럽트 커맨드 레지스터(64 비트) - 벡터(0~7)
#define APIC_VECTOR_KERNEL32STARTADDRESS 0x10 // 0x10=0x10000/4KB : 보호모드 커널 시작 어드레스

/***** 함수 정의 *****/
QWORD kGetLocalAPICBaseAddress(void);
void kEnableSoftwareLocalAPIC(void);


#endif