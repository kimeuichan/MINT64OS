#ifndef __RTC_H__
#define __RTC_H__

#include "Types.h"

/***** ��ũ�� ���� *****/
// RTC I/O ��Ʈ
#define RTC_CMOSADDRESS 0x70 // CMOS �޸� ��巹�� ��������
#define RTC_CMOSDATA    0x71 // CMOS �޸� ������ ��������

// CMOS �޸� ��巹��(1byte)�� �ʵ�
#define RTC_ADDRESS_SECOND     0x00 // Seconds ��������            : ���� �ð� �߿��� �ʸ� �����ϴ� ��������
#define RTC_ADDRESS_MINUTE     0x02 // Minutes ��������            : ���� �ð� �߿��� ���� �����ϴ� ��������
#define RTC_ADDRESS_HOUR       0x04 // Hours ��������                 : ���� �ð� �߿��� �ð��� �����ϴ� ��������
#define RTC_ADDRESS_DAYOFWEEK  0x06 // Day Of Week ��������   : ���� ��¥ �߿��� ������ �����ϴ� �������� (1~7:��-��-ȭ-��-��-��-��)
#define RTC_ADDRESS_DAYOFMONTH 0x07 // Day Of Month �������� : ���� ��¥ �߿��� ���� �����ϴ� ��������
#define RTC_ADDRESS_MONTH      0x08 // Month ��������                  : ���� ��¥ �߿��� ���� �����ϴ� ��������
#define RTC_ADDRESS_YEAR       0x09 // Year ��������                    : ���� ��¥ �߿��� ���� �����ϴ� ��������

// BCD����->Binary���� ��ȯ ��ũ��
#define RTC_BCDTOBINARY(x) ((((x) >> 4) * 10) + ((x) & 0x0F))

/***** �Լ� ���� *****/
void kReadRTCTime(BYTE* pbHour, BYTE* pbMinute, BYTE* pbSecond);
void kReadRTCDate(WORD* pwYear, BYTE* pbMonth, BYTE* pbDayOfMonth, BYTE* pbDayOfWeek);
char* kConvertDayOfWeekToString(BYTE bDayOfWeek);

#endif // __RTC_H__
