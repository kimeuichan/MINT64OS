#include "Utility.h"
#include "AssemblyUtility.h"

// PIT 컨트롤러가 발생한 횟수를 저장할 카운터
volatile QWORD g_qwTickCount = 0;

void kMemSet(void* pvDest, BYTE bData, int iSize){
	int i;

	for(i = 0; i < iSize; i++){
		((char*)pvDest)[i] = bData;
	}
}

int kMemCpy(void* pvDest, const void* pvSrc, int iSize){
	int i;

	for(i = 0; i < iSize; i++){
		((char*)pvDest)[i] = ((char*)pvSrc)[i];
	}

	return iSize;
}

int kMemCmp(const void* pvDest, const void* pvSrc, int iSize){
	int i;
	char cTemp;

	for(i = 0; i < iSize; i++){
		cTemp = ((char*)pvDest)[i] - ((char*)pvSrc)[i];
		if(cTemp != 0){
			return (int)cTemp;
		}
	}

	return 0;
}

BOOL kSetInterruptFlag(BOOL bEnableInterrupt){
	QWORD qwRFLAGS;

	qwRFLAGS = kReadRFLAGS();

	if(bEnableInterrupt == TRUE){
		kEnableInterrupt();

	}else{
		kDisableInterrupt();
	}

	// RFLAGS 레지스터의 IF(비트 9) 필드를 확인하여 이전의 인터럽트 상태를 반환
	if(qwRFLAGS & 0x0200){
		return TRUE;
	}

	return FALSE;
}

int kStrLen(const char* pcBuffer){
	int i;

	for(i = 0; ; i++){
		if(pcBuffer[i] == '\0'){
			break;
		}
	}

	return i;
}

/***** 전역 변수 정의 *****/
static QWORD gs_qwTotalRAMMBSize = 0; // RAM의 총 크기(MB단위)

void kCheckTotalRAMSize(void){
	DWORD* pdwCurrentAddress;
	DWORD dwPreviousValue;

	// 64MB위치부터 4MB단위로 4B를 체크
	pdwCurrentAddress = (DWORD*)0x4000000;

	while(1){
		dwPreviousValue = *pdwCurrentAddress;

		// 4B 체크
		*pdwCurrentAddress = 0x12345678;
		if(*pdwCurrentAddress != 0x12345678){
			break;
		}

		*pdwCurrentAddress = dwPreviousValue;

		// 4MB 이동
		pdwCurrentAddress += (0x400000 / 4);

	}

	// MB단위로 계산
	gs_qwTotalRAMMBSize = (QWORD)pdwCurrentAddress / 0x100000;
}

QWORD kGetTotalRAMSize(void){
	kCheckTotalRAMSize(); // 임시방편으로 추가
	return gs_qwTotalRAMMBSize;
}

long kAToI(const char* pcBuffer, int iRadix){
	long lReturn;

	switch(iRadix){
	case 16:
		lReturn = kHexStringToQword(pcBuffer);
		break;

	case 10:
	default:
		lReturn = kDecimalStringToLong(pcBuffer);
		break;
	}

	return lReturn;
}

QWORD kHexStringToQword(const char* pcBuffer){
	QWORD qwValue = 0;
	int i;

	for(i = 0; pcBuffer[i] != '\0'; i++){
		qwValue *= 16;

		if('A' <= pcBuffer[i] && pcBuffer[i] <= 'Z'){
			qwValue += (pcBuffer[i] - 'A') + 10;

		}else if('a' <= pcBuffer[i] && pcBuffer[i] <= 'z'){
			qwValue += (pcBuffer[i] - 'a') + 10;

		}else{
			qwValue += (pcBuffer[i] - '0');
		}
	}

	return qwValue;
}

long kDecimalStringToLong(const char* pcBuffer){
	long lValue = 0;
	int i;

	if(pcBuffer[0] == '-'){
		i = 1;

	}else{
		i = 0;
	}

	for( ; pcBuffer[i] != '\0'; i++){
		lValue *= 10;
		lValue += (pcBuffer[i] - '0');
	}

	if(pcBuffer[0] == '-'){
		lValue = -lValue;
	}

	return lValue;
}

int kIToA(long lValue, char* pcBuffer, int iRadix){
	int iReturn;

	switch(iRadix){
	case 16:
		iReturn = kHexToString(lValue, pcBuffer);
		break;

	case 10:
	default:
		iReturn = kDecimalToString(lValue, pcBuffer);
		break;
	}

	return iReturn;
}

int kHexToString(QWORD pwValue, char* pcBuffer){
	QWORD i;
	QWORD qwCurrentValue;

	if(pwValue == 0){
		pcBuffer[0] = '0';
		pcBuffer[1] = '\0';
		return 1;
	}

	for(i = 0; pwValue > 0; i++){
		qwCurrentValue = pwValue % 16;

		if(qwCurrentValue >= 10){
			pcBuffer[i] = (qwCurrentValue - 10) + 'A';

		}else{
			pcBuffer[i] = qwCurrentValue + '0';
		}

		pwValue /= 16;
	}

	pcBuffer[i] = '\0';

	kReverseString(pcBuffer);

	return i;
}

int kDecimalToString(long lValue, char* pcBuffer){
	long i;

	if(lValue == 0){
		pcBuffer[0] = '0';
		pcBuffer[1] = '\0';
		return 1;
	}

	if(lValue < 0){
		i = 1;
		pcBuffer[0] = '-';
		lValue = -lValue;

	}else{
		i = 0;
	}

	for( ; lValue > 0; i++){
		pcBuffer[i] = (lValue % 10) + '0';
		lValue /= 10;
	}

	pcBuffer[i] = '\0';

	if(pcBuffer[0] == '-'){
		kReverseString(&(pcBuffer[1]));

	}else{
		kReverseString(pcBuffer);
	}

	return i;
}

void kReverseString(char* pcBuffer){
	int iLength;
	int i;
	char cTemp;

	iLength = kStrLen(pcBuffer);

	for(i = 0; i < (iLength / 2); i++){
		cTemp = pcBuffer[i];
		pcBuffer[i] = pcBuffer[iLength - 1 - i];
		pcBuffer[iLength - 1 - i] = cTemp;
	}
}

int kSPrintf(char* pcBuffer, const char* pcFormatString, ...){
	va_list ap;
	int iReturn;

	va_start(ap, pcFormatString);
	iReturn = kVSPrintf(pcBuffer, pcFormatString, ap);
	va_end(ap);

	return iReturn; // 출력한 문자열의 길이를 반환
}

/* kVSPrintf 함수가 지원하는 데이터 타입
 * %s         : string
 * %c         : char
 * %d, %i     : decimal int (10진수, signed)
 * %x, %X     : DWORD (16진수, unsigned)
 * %q, %Q, %p : QWORD (16진수, unsigned)
 */
int kVSPrintf(char* pcBuffer, const char* pcFormatString, va_list ap){
	QWORD i, j, k;
	int iBufferIndex = 0;
	int iFormatLength, iCopyLength;
	char* pcCopyString;
	QWORD qwValue;
	int iValue;

	iFormatLength = kStrLen(pcFormatString);

	for(i = 0; i < iFormatLength; i++){
		// '%'인 경우 (데이터 타입)
		if(pcFormatString[i] == '%'){
			i++;
			switch(pcFormatString[i]){
			case 's':
				pcCopyString = (char*)(va_arg(ap, char*));
				iCopyLength = kStrLen(pcCopyString);
				kMemCpy(pcBuffer + iBufferIndex, pcCopyString, iCopyLength);
				iBufferIndex += iCopyLength;
				break;

			case 'c':
				pcBuffer[iBufferIndex] = (char)(va_arg(ap, int));
				iBufferIndex++;
				break;

			case 'd':
			case 'i':
				iValue = (int)(va_arg(ap, int));
				iBufferIndex += kIToA(iValue, pcBuffer + iBufferIndex, 10);
				break;

			case 'x':
			case 'X':
				qwValue = (DWORD)(va_arg(ap, DWORD)) & 0xFFFFFFFF;
				iBufferIndex += kIToA(qwValue, pcBuffer + iBufferIndex, 16);
				break;

			case 'q':
			case 'Q':
			case 'p':
				qwValue = (QWORD)(va_arg(ap, QWORD));
				iBufferIndex += kIToA(qwValue, pcBuffer + iBufferIndex, 16);
				break;

			case 'f':
				dValue = (QWORD)(va_arg(ap, double));
				// 셋째 자리에서 반올림 처리
				dValue += 0.005;
				// 소수 둘째 자리부터 차례로 저장하여 버퍼를 뒤집음
				pcBuffer[iBufferIndex] = '0'+(QWORD)(dValue*100)%10;
				pcBuffer[iBufferIndex+1] = '0'+(QWORD)(dValue*10)%10;
				pcBuffer[iBufferIndex+2] = '.';
				for(k=0; ;k++){
					if(((QWORD)dValue == 0) && (k != 0))
						break;
					pcBuffer[iBufferIndex+3+k] = '0'+((QWORD)dValue%10);
					dValue = dValue/10;
				}
				pcBuffer[iBufferIndex+3+k] = '\0';
				// 값이 저장된 길이만큼 뒤집고 길이를 증가시킴
				kReverseString(pcBuffer+iBufferIndex);
				iBufferIndex += 3+k;
				break;


			default:
				pcBuffer[iBufferIndex] = pcFormatString[i];
				iBufferIndex++;
				break;
			}

		// '%'가 아닌 경우 (일반 문자열)
		}else{
			pcBuffer[iBufferIndex] = pcFormatString[i];
			iBufferIndex++;
		}
	}

	pcBuffer[iBufferIndex] = '\0';

	return iBufferIndex; // 출력한 문자열의 길이를 반환
}

QWORD kGetTickCount(void){
	return g_qwTickCount;
}
void kSleep(QWORD qwMillisecond){
	QWORD qwLastTickCount;

	qwLastTickCount = g_qwTickCount;

	while((g_qwTickCount - qwLastTickCount) <= qwMillisecond){
		kSchedule();
	}
}