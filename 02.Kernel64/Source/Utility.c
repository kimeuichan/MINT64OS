#include "Utility.h"
#include "AssemblyUtility.h"

/***** 전역 변수 정의 *****/
volatile QWORD g_qwTickCount = 0; // Timer(IRQ0, PIT 컨트롤러)에서 인터럽트가 발생한 횟수를 저장

//====================================================================================================
// 메모리 초기화, 복사, 비교 (1 byte 단위)
//====================================================================================================

//////////////////////////////////////////////////////////////////////////////////////////////////////
//void kMemSet(void* pvDest, BYTE bData, int iSize){
//	int i;
//
//	for(i = 0; i < iSize; i++){
//		((char*)pvDest)[i] = bData;
//	}
//}
//
//int kMemCpy(void* pvDest, const void* pvSrc, int iSize){
//	int i;
//
//	for(i = 0; i < iSize; i++){
//		((char*)pvDest)[i] = ((char*)pvSrc)[i];
//	}
//
//	return iSize;
//}
//
//int kMemCmp(const void* pvDest, const void* pvSrc, int iSize){
//	int i;
//	char cTemp;
//
//	for(i = 0; i < iSize; i++){
//		cTemp = ((char*)pvDest)[i] - ((char*)pvSrc)[i];
//		if(cTemp != 0){
//			return (int)cTemp;
//		}
//	}
//
//	return 0;
//}
//////////////////////////////////////////////////////////////////////////////////////////////////////

//====================================================================================================
// 메모리 초기화, 복사, 비교 (8 byte 단위 : 64비트 모드는 범용 레지스터의 크기가 8바이트이기 때문임)
//====================================================================================================

void kMemSet(void* pvDest, BYTE bData, int iSize){
	int i;
	QWORD qwData;
	int iRemainByteStartOffset;

	// 8바이트 데이터를 만듦
	qwData = 0;
	for(i = 0; i < 8; i++){
		qwData = (qwData << 8) | bData;
	}

	// 8바이트 단위로 메모리 초기화
	for(i = 0; i < (iSize / 8); i++){
		((QWORD*)pvDest)[i] = qwData;
	}

	// 남은 부분을 1바이트 단위로 메모리 초기화
	iRemainByteStartOffset = i * 8;
	for(i = 0; i < (iSize % 8); i++){
		((char*)pvDest)[iRemainByteStartOffset++] = bData;
	}
}

int kMemCpy(void* pvDest, const void* pvSrc, int iSize){
	int i;
	int iRemainByteStartOffset;

	// 8바이트 단위로 메모리 복사
	for(i = 0; i < (iSize / 8); i++){
		((QWORD*)pvDest)[i] = ((QWORD*)pvSrc)[i];
	}

	// 남은 부분을 1바이트 단위로 메모리 복사
	iRemainByteStartOffset = i * 8;
	for(i = 0; i < (iSize % 8); i++){
		((char*)pvDest)[iRemainByteStartOffset] = ((char*)pvSrc)[iRemainByteStartOffset];
		iRemainByteStartOffset++;
	}

	// 복사한 메모리 크기를 반환
	return iSize;
}

int kMemCmp(const void* pvDest, const void* pvSrc, int iSize){
	int i, j;
	int iRemainByteStartOffset;
	QWORD qwValue;
	char cValue;

	// 8바이트 단위로 메모리 비교
	for(i = 0; i < (iSize / 8); i++){
		qwValue = ((QWORD*)pvDest)[i] - ((QWORD*)pvSrc)[i];
		if(qwValue != 0){

			// 8바이트 값에서 1바이트 단위로 다른 위치를 정확하게 찾아서 그 값을 반환
			for(j = 0; j < 8; j++){
				if(((qwValue >> (j * 8)) & 0xFF) != 0){
					return (qwValue >> (j * 8)) & 0xFF;
				}
			}
		}
	}

	// 남은 부분을 1바이트 단위로 메모리 비교
	iRemainByteStartOffset = i * 8;
	for(i = 0; i < (iSize % 8); i++){
		cValue = ((char*)pvDest)[iRemainByteStartOffset] - ((char*)pvSrc)[iRemainByteStartOffset];
		if(cValue != 0){
			return cValue;
		}

		iRemainByteStartOffset++;
	}

	// 비교 결과가 일치하는 경우, 0을 리턴
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

//****************************************************************************************************
// <<kVSPrintf 함수가 지원하는 데이터 타입>>
//----------------------------------------------------------------------------------------------------
// %s         : string
// %c         : char
// %d, %i     : decimal int (10진수, signed)
// %x, %X     : DWORD (16진수, unsigned)
// %q, %Q, %p : QWORD (16진수, unsigned)
// %f         : float (소수점 셋째 자리에서 반올림하여 소수점 둘째 자리까지 출력)
//****************************************************************************************************
int kVSPrintf(char* pcBuffer, const char* pcFormatString, va_list ap){
	QWORD i, j, k;
	int iBufferIndex = 0;
	int iFormatLength, iCopyLength;
	char* pcCopyString;
	QWORD qwValue;
	int iValue;
	double dValue;

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
				dValue = (double)(va_arg(ap, double));

				// 소수점 셋째 자리에서 반올림
				dValue += 0.005;

				// 소수 부분의 소수점 둘째 자리부터 차례대로 버퍼에 저장
				pcBuffer[iBufferIndex] = '0' + ((QWORD)(dValue * 100) % 10);
				pcBuffer[iBufferIndex + 1] = '0' + ((QWORD)(dValue * 10) % 10);
				pcBuffer[iBufferIndex + 2] = '.';

				for(k = 0; ; k++){
					// 정수 부분이 0이면, 루프 종료
					if(((QWORD)dValue == 0) && (k != 0)){
						break;
					}

					// 정수 부분의 일의 자리부터 차례대로 버퍼에 저장
					pcBuffer[iBufferIndex + 3 + k] = '0' + ((QWORD)dValue % 10);
					dValue = dValue / 10;
				}

				pcBuffer[iBufferIndex + 3 + k] = '\0';

				// 실수가 저장된 길이 만큼 버퍼를 뒤집고, 버퍼 인덱스를 증가시킴
				kReverseString(pcBuffer + iBufferIndex);
				iBufferIndex += 3 + k;
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
