#include "Utility.h"
#include "AssemblyUtility.h"

/***** ���� ���� ���� *****/
volatile QWORD g_qwTickCount = 0; // Timer(IRQ0, PIT ��Ʈ�ѷ�)���� ���ͷ�Ʈ�� �߻��� Ƚ���� ����

//====================================================================================================
// �޸� �ʱ�ȭ, ����, �� (1 byte ����)
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
// �޸� �ʱ�ȭ, ����, �� (8 byte ���� : 64��Ʈ ���� ���� ���������� ũ�Ⱑ 8����Ʈ�̱� ������)
//====================================================================================================

void kMemSet(void* pvDest, BYTE bData, int iSize){
	int i;
	QWORD qwData;
	int iRemainByteStartOffset;

	// 8����Ʈ �����͸� ����
	qwData = 0;
	for(i = 0; i < 8; i++){
		qwData = (qwData << 8) | bData;
	}

	// 8����Ʈ ������ �޸� �ʱ�ȭ
	for(i = 0; i < (iSize / 8); i++){
		((QWORD*)pvDest)[i] = qwData;
	}

	// ���� �κ��� 1����Ʈ ������ �޸� �ʱ�ȭ
	iRemainByteStartOffset = i * 8;
	for(i = 0; i < (iSize % 8); i++){
		((char*)pvDest)[iRemainByteStartOffset++] = bData;
	}
}

int kMemCpy(void* pvDest, const void* pvSrc, int iSize){
	int i;
	int iRemainByteStartOffset;

	// 8����Ʈ ������ �޸� ����
	for(i = 0; i < (iSize / 8); i++){
		((QWORD*)pvDest)[i] = ((QWORD*)pvSrc)[i];
	}

	// ���� �κ��� 1����Ʈ ������ �޸� ����
	iRemainByteStartOffset = i * 8;
	for(i = 0; i < (iSize % 8); i++){
		((char*)pvDest)[iRemainByteStartOffset] = ((char*)pvSrc)[iRemainByteStartOffset];
		iRemainByteStartOffset++;
	}

	// ������ �޸� ũ�⸦ ��ȯ
	return iSize;
}

int kMemCmp(const void* pvDest, const void* pvSrc, int iSize){
	int i, j;
	int iRemainByteStartOffset;
	QWORD qwValue;
	char cValue;

	// 8����Ʈ ������ �޸� ��
	for(i = 0; i < (iSize / 8); i++){
		qwValue = ((QWORD*)pvDest)[i] - ((QWORD*)pvSrc)[i];
		if(qwValue != 0){

			// 8����Ʈ ������ 1����Ʈ ������ �ٸ� ��ġ�� ��Ȯ�ϰ� ã�Ƽ� �� ���� ��ȯ
			for(j = 0; j < 8; j++){
				if(((qwValue >> (j * 8)) & 0xFF) != 0){
					return (qwValue >> (j * 8)) & 0xFF;
				}
			}
		}
	}

	// ���� �κ��� 1����Ʈ ������ �޸� ��
	iRemainByteStartOffset = i * 8;
	for(i = 0; i < (iSize % 8); i++){
		cValue = ((char*)pvDest)[iRemainByteStartOffset] - ((char*)pvSrc)[iRemainByteStartOffset];
		if(cValue != 0){
			return cValue;
		}

		iRemainByteStartOffset++;
	}

	// �� ����� ��ġ�ϴ� ���, 0�� ����
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

	// RFLAGS ���������� IF(��Ʈ 9) �ʵ带 Ȯ���Ͽ� ������ ���ͷ�Ʈ ���¸� ��ȯ
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

/***** ���� ���� ���� *****/
static QWORD gs_qwTotalRAMMBSize = 0; // RAM�� �� ũ��(MB����)

void kCheckTotalRAMSize(void){
	DWORD* pdwCurrentAddress;
	DWORD dwPreviousValue;

	// 64MB��ġ���� 4MB������ 4B�� üũ
	pdwCurrentAddress = (DWORD*)0x4000000;

	while(1){
		dwPreviousValue = *pdwCurrentAddress;

		// 4B üũ
		*pdwCurrentAddress = 0x12345678;
		if(*pdwCurrentAddress != 0x12345678){
			break;
		}

		*pdwCurrentAddress = dwPreviousValue;

		// 4MB �̵�
		pdwCurrentAddress += (0x400000 / 4);

	}

	// MB������ ���
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

	return iReturn; // ����� ���ڿ��� ���̸� ��ȯ
}

//****************************************************************************************************
// <<kVSPrintf �Լ��� �����ϴ� ������ Ÿ��>>
//----------------------------------------------------------------------------------------------------
// %s         : string
// %c         : char
// %d, %i     : decimal int (10����, signed)
// %x, %X     : DWORD (16����, unsigned)
// %q, %Q, %p : QWORD (16����, unsigned)
// %f         : float (�Ҽ��� ��° �ڸ����� �ݿø��Ͽ� �Ҽ��� ��° �ڸ����� ���)
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
		// '%'�� ��� (������ Ÿ��)
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

				// �Ҽ��� ��° �ڸ����� �ݿø�
				dValue += 0.005;

				// �Ҽ� �κ��� �Ҽ��� ��° �ڸ����� ���ʴ�� ���ۿ� ����
				pcBuffer[iBufferIndex] = '0' + ((QWORD)(dValue * 100) % 10);
				pcBuffer[iBufferIndex + 1] = '0' + ((QWORD)(dValue * 10) % 10);
				pcBuffer[iBufferIndex + 2] = '.';

				for(k = 0; ; k++){
					// ���� �κ��� 0�̸�, ���� ����
					if(((QWORD)dValue == 0) && (k != 0)){
						break;
					}

					// ���� �κ��� ���� �ڸ����� ���ʴ�� ���ۿ� ����
					pcBuffer[iBufferIndex + 3 + k] = '0' + ((QWORD)dValue % 10);
					dValue = dValue / 10;
				}

				pcBuffer[iBufferIndex + 3 + k] = '\0';

				// �Ǽ��� ����� ���� ��ŭ ���۸� ������, ���� �ε����� ������Ŵ
				kReverseString(pcBuffer + iBufferIndex);
				iBufferIndex += 3 + k;
				break;

			default:
				pcBuffer[iBufferIndex] = pcFormatString[i];
				iBufferIndex++;
				break;
			}

		// '%'�� �ƴ� ��� (�Ϲ� ���ڿ�)
		}else{
			pcBuffer[iBufferIndex] = pcFormatString[i];
			iBufferIndex++;
		}
	}

	pcBuffer[iBufferIndex] = '\0';

	return iBufferIndex; // ����� ���ڿ��� ���̸� ��ȯ
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
