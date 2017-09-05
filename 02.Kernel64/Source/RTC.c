#include "RTC.h"
#include "AssemblyUtility.h"

void kReadRTCTime(BYTE* pbHour, BYTE* pbMinute, BYTE* pbSecond){
	BYTE bData;

	// CMOS �޸� ��巹�� �������Ϳ� Hours �������͸� �����ϰ�, CMOS �޸� ������ �������Ϳ��� ���� �ð��� ����
	kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_HOUR);
	bData = kInPortByte(RTC_CMOSDATA);
	*pbHour = RTC_BCDTOBINARY(bData);

	// CMOS �޸� ��巹�� �������Ϳ� Minutes �������͸� �����ϰ�, CMOS �޸� ������ �������Ϳ��� ���� ���� ����
	kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_MINUTE);
	bData = kInPortByte(RTC_CMOSDATA);
	*pbMinute = RTC_BCDTOBINARY(bData);

	// CMOS �޸� ��巹�� �������Ϳ� Seconds �������͸� �����ϰ�, CMOS �޸� ������ �������Ϳ��� ���� ���� ����
	kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_SECOND);
	bData = kInPortByte(RTC_CMOSDATA);
	*pbSecond = RTC_BCDTOBINARY(bData);
}

void kReadRTCDate(WORD* pwYear, BYTE* pbMonth, BYTE* pbDayOfMonth, BYTE* pbDayOfWeek){
	BYTE bData;

	// CMOS �޸� ��巹�� �������Ϳ� Year �������͸� �����ϰ�, CMOS �޸� ������ �������Ϳ��� ���� ���� ����
	kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_YEAR);
	bData = kInPortByte(RTC_CMOSDATA);
	*pwYear = RTC_BCDTOBINARY(bData) + 2000;

	// CMOS �޸� ��巹�� �������Ϳ� Month �������͸� �����ϰ�, CMOS �޸� ������ �������Ϳ��� ���� ���� ����
	kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_MONTH);
	bData = kInPortByte(RTC_CMOSDATA);
	*pbMonth = RTC_BCDTOBINARY(bData);

	// CMOS �޸� ��巹�� �������Ϳ� Day Of Month �������͸� �����ϰ�, CMOS �޸� ������ �������Ϳ��� ���� ���� ����
	kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_DAYOFMONTH);
	bData = kInPortByte(RTC_CMOSDATA);
	*pbDayOfMonth = RTC_BCDTOBINARY(bData);

	// CMOS �޸� ��巹�� �������Ϳ� Day Of Week �������͸� �����ϰ�, CMOS �޸� ������ �������Ϳ��� ���� ������ ����
	kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_DAYOFWEEK);
	bData = kInPortByte(RTC_CMOSDATA);
	*pbDayOfWeek = RTC_BCDTOBINARY(bData);
}

char* kConvertDayOfWeekToString(BYTE bDayOfWeek){
	// bDayOfWeek=[1~7:��-��-ȭ-��-��-��-��]
	static char* vpcDayOfWeekString[8] = {"Error", "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

	if(bDayOfWeek >= 8){
		return vpcDayOfWeekString[0];
	}

	return vpcDayOfWeekString[bDayOfWeek];
}
