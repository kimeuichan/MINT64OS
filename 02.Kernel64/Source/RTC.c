#include "RTC.h"
#include "AssemblyUtility.h"

void kReadRTCTime(BYTE* pbHour, BYTE* pbMinute, BYTE* pbSecond){
	BYTE bData;

	// CMOS 메모리 어드레스 레지스터에 Hours 레지스터를 설정하고, CMOS 메모리 데이터 레지스터에서 현재 시간을 읽음
	kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_HOUR);
	bData = kInPortByte(RTC_CMOSDATA);
	*pbHour = RTC_BCDTOBINARY(bData);

	// CMOS 메모리 어드레스 레지스터에 Minutes 레지스터를 설정하고, CMOS 메모리 데이터 레지스터에서 현재 분을 읽음
	kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_MINUTE);
	bData = kInPortByte(RTC_CMOSDATA);
	*pbMinute = RTC_BCDTOBINARY(bData);

	// CMOS 메모리 어드레스 레지스터에 Seconds 레지스터를 설정하고, CMOS 메모리 데이터 레지스터에서 현재 초을 읽음
	kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_SECOND);
	bData = kInPortByte(RTC_CMOSDATA);
	*pbSecond = RTC_BCDTOBINARY(bData);
}

void kReadRTCDate(WORD* pwYear, BYTE* pbMonth, BYTE* pbDayOfMonth, BYTE* pbDayOfWeek){
	BYTE bData;

	// CMOS 메모리 어드레스 레지스터에 Year 레지스터를 설정하고, CMOS 메모리 데이터 레지스터에서 현재 연을 읽음
	kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_YEAR);
	bData = kInPortByte(RTC_CMOSDATA);
	*pwYear = RTC_BCDTOBINARY(bData) + 2000;

	// CMOS 메모리 어드레스 레지스터에 Month 레지스터를 설정하고, CMOS 메모리 데이터 레지스터에서 현재 월을 읽음
	kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_MONTH);
	bData = kInPortByte(RTC_CMOSDATA);
	*pbMonth = RTC_BCDTOBINARY(bData);

	// CMOS 메모리 어드레스 레지스터에 Day Of Month 레지스터를 설정하고, CMOS 메모리 데이터 레지스터에서 현재 일을 읽음
	kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_DAYOFMONTH);
	bData = kInPortByte(RTC_CMOSDATA);
	*pbDayOfMonth = RTC_BCDTOBINARY(bData);

	// CMOS 메모리 어드레스 레지스터에 Day Of Week 레지스터를 설정하고, CMOS 메모리 데이터 레지스터에서 현재 요일을 읽음
	kOutPortByte(RTC_CMOSADDRESS, RTC_ADDRESS_DAYOFWEEK);
	bData = kInPortByte(RTC_CMOSDATA);
	*pbDayOfWeek = RTC_BCDTOBINARY(bData);
}

char* kConvertDayOfWeekToString(BYTE bDayOfWeek){
	// bDayOfWeek=[1~7:일-월-화-수-목-금-토]
	static char* vpcDayOfWeekString[8] = {"Error", "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

	if(bDayOfWeek >= 8){
		return vpcDayOfWeekString[0];
	}

	return vpcDayOfWeekString[bDayOfWeek];
}
