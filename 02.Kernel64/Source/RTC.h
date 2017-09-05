#ifndef __RTC_H__
#define __RTC_H__

#include "Types.h"

/***** 매크로 정의 *****/
// RTC I/O 포트
#define RTC_CMOSADDRESS 0x70 // CMOS 메모리 어드레스 레지스터
#define RTC_CMOSDATA    0x71 // CMOS 메모리 데이터 레지스터

// CMOS 메모리 어드레스(1byte)의 필드
#define RTC_ADDRESS_SECOND     0x00 // Seconds 레지스터            : 현재 시간 중에서 초를 저장하는 레지스터
#define RTC_ADDRESS_MINUTE     0x02 // Minutes 레지스터            : 현재 시간 중에서 분을 저장하는 레지스터
#define RTC_ADDRESS_HOUR       0x04 // Hours 레지스터                 : 현재 시간 중에서 시간을 저장하는 레지스터
#define RTC_ADDRESS_DAYOFWEEK  0x06 // Day Of Week 레지스터   : 현재 날짜 중에서 요일을 저장하는 레지스터 (1~7:일-월-화-수-목-금-토)
#define RTC_ADDRESS_DAYOFMONTH 0x07 // Day Of Month 레지스터 : 현재 날짜 중에서 일을 저장하는 레지스터
#define RTC_ADDRESS_MONTH      0x08 // Month 레지스터                  : 현재 날짜 중에서 월을 저장하는 레지스터
#define RTC_ADDRESS_YEAR       0x09 // Year 레지스터                    : 현재 날짜 중에서 연을 저장하는 레지스터

// BCD포맷->Binary포맷 변환 매크로
#define RTC_BCDTOBINARY(x) ((((x) >> 4) * 10) + ((x) & 0x0F))

/***** 함수 정의 *****/
void kReadRTCTime(BYTE* pbHour, BYTE* pbMinute, BYTE* pbSecond);
void kReadRTCDate(WORD* pwYear, BYTE* pbMonth, BYTE* pbDayOfMonth, BYTE* pbDayOfWeek);
char* kConvertDayOfWeekToString(BYTE bDayOfWeek);

#endif // __RTC_H__
