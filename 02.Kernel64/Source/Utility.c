#include "Utility.h"
#include "AssemblyUtility.h"

/***** Àü¿ª º¯¼ö Á¤ÀÇ *****/
volatile QWORD g_qwTickCount = 0; // Timer(IRQ0, PIT ÄÁÆ®·Ñ·¯)¿¡¼­ ÀÎÅÍ·´Æ®°¡ ¹ß»ýÇÑ È½¼ö¸¦ ÀúÀå

//====================================================================================================
// ¸Þ¸ð¸® ÃÊ±âÈ­, º¹»ç, ºñ±³ (1 byte ´ÜÀ§)
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
// ¸Þ¸ð¸® ÃÊ±âÈ­, º¹»ç, ºñ±³ (8 byte ´ÜÀ§ : 64ºñÆ® ¸ðµå´Â ¹ü¿ë ·¹Áö½ºÅÍÀÇ Å©±â°¡ 8¹ÙÀÌÆ®ÀÌ±â ¶§¹®ÀÓ)
//====================================================================================================

void kMemSet(void* pvDest, BYTE bData, int iSize){
	int i;
	QWORD qwData;
	int iRemainByteStartOffset;

	// 8¹ÙÀÌÆ® µ¥ÀÌÅÍ¸¦ ¸¸µê
	qwData = 0;
	for(i = 0; i < 8; i++){
		qwData = (qwData << 8) | bData;
	}

	// 8¹ÙÀÌÆ® ´ÜÀ§·Î ¸Þ¸ð¸® ÃÊ±âÈ­
	for(i = 0; i < (iSize / 8); i++){
		((QWORD*)pvDest)[i] = qwData;
	}

	// ³²Àº ºÎºÐÀ» 1¹ÙÀÌÆ® ´ÜÀ§·Î ¸Þ¸ð¸® ÃÊ±âÈ­
	iRemainByteStartOffset = i * 8;
	for(i = 0; i < (iSize % 8); i++){
		((char*)pvDest)[iRemainByteStartOffset++] = bData;
	}
}

int kMemCpy(void* pvDest, const void* pvSrc, int iSize){
	int i;
	int iRemainByteStartOffset;

	// 8¹ÙÀÌÆ® ´ÜÀ§·Î ¸Þ¸ð¸® º¹»ç
	for(i = 0; i < (iSize / 8); i++){
		((QWORD*)pvDest)[i] = ((QWORD*)pvSrc)[i];
	}

	// ³²Àº ºÎºÐÀ» 1¹ÙÀÌÆ® ´ÜÀ§·Î ¸Þ¸ð¸® º¹»ç
	iRemainByteStartOffset = i * 8;
	for(i = 0; i < (iSize % 8); i++){
		((char*)pvDest)[iRemainByteStartOffset] = ((char*)pvSrc)[iRemainByteStartOffset];
		iRemainByteStartOffset++;
	}

	// º¹»çÇÑ ¸Þ¸ð¸® Å©±â¸¦ ¹ÝÈ¯
	return iSize;
}

int kMemCmp(const void* pvDest, const void* pvSrc, int iSize){
	int i, j;
	int iRemainByteStartOffset;
	QWORD qwValue;
	char cValue;

	// 8¹ÙÀÌÆ® ´ÜÀ§·Î ¸Þ¸ð¸® ºñ±³
	for(i = 0; i < (iSize / 8); i++){
		qwValue = ((QWORD*)pvDest)[i] - ((QWORD*)pvSrc)[i];
		if(qwValue != 0){

			// 8¹ÙÀÌÆ® °ª¿¡¼­ 1¹ÙÀÌÆ® ´ÜÀ§·Î ´Ù¸¥ À§Ä¡¸¦ Á¤È®ÇÏ°Ô Ã£¾Æ¼­ ±× °ªÀ» ¹ÝÈ¯
			for(j = 0; j < 8; j++){
				if(((qwValue >> (j * 8)) & 0xFF) != 0){
					return (qwValue >> (j * 8)) & 0xFF;
				}
			}
		}
	}

	// ³²Àº ºÎºÐÀ» 1¹ÙÀÌÆ® ´ÜÀ§·Î ¸Þ¸ð¸® ºñ±³
	iRemainByteStartOffset = i * 8;
	for(i = 0; i < (iSize % 8); i++){
		cValue = ((char*)pvDest)[iRemainByteStartOffset] - ((char*)pvSrc)[iRemainByteStartOffset];
		if(cValue != 0){
			return cValue;
		}

		iRemainByteStartOffset++;
	}

	// ºñ±³ °á°ú°¡ ÀÏÄ¡ÇÏ´Â °æ¿ì, 0À» ¸®ÅÏ
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

	// RFLAGS ·¹Áö½ºÅÍÀÇ IF(ºñÆ® 9) ÇÊµå¸¦ È®ÀÎÇÏ¿© ÀÌÀüÀÇ ÀÎÅÍ·´Æ® »óÅÂ¸¦ ¹ÝÈ¯
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

/***** Àü¿ª º¯¼ö Á¤ÀÇ *****/
static QWORD gs_qwTotalRAMMBSize = 0; // RAMÀÇ ÃÑ Å©±â(MB´ÜÀ§)

void kCheckTotalRAMSize(void){
	DWORD* pdwCurrentAddress;
	DWORD dwPreviousValue;

	// 64MBÀ§Ä¡ºÎÅÍ 4MB´ÜÀ§·Î 4B¸¦ Ã¼Å©
	pdwCurrentAddress = (DWORD*)0x4000000;

	while(1){
		dwPreviousValue = *pdwCurrentAddress;

		// 4B Ã¼Å©
		*pdwCurrentAddress = 0x12345678;
		if(*pdwCurrentAddress != 0x12345678){
			break;
		}

		*pdwCurrentAddress = dwPreviousValue;

		// 4MB ÀÌµ¿
		pdwCurrentAddress += (0x400000 / 4);

	}

	// MB´ÜÀ§·Î °è»ê
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

	return iReturn; // Ãâ·ÂÇÑ ¹®ÀÚ¿­ÀÇ ±æÀÌ¸¦ ¹ÝÈ¯
}

//****************************************************************************************************
// <<kVSPrintf ÇÔ¼ö°¡ Áö¿øÇÏ´Â µ¥ÀÌÅÍ Å¸ÀÔ>>
//----------------------------------------------------------------------------------------------------
// %s         : string
// %c         : char
// %d, %i     : decimal int (10Áø¼ö, signed)
// %x, %X     : DWORD (16Áø¼ö, unsigned)
// %q, %Q, %p : QWORD (16Áø¼ö, unsigned)
// %f         : float (¼Ò¼öÁ¡ ¼ÂÂ° ÀÚ¸®¿¡¼­ ¹Ý¿Ã¸²ÇÏ¿© ¼Ò¼öÁ¡ µÑÂ° ÀÚ¸®±îÁö Ãâ·Â)
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
		// '%'ÀÎ °æ¿ì (µ¥ÀÌÅÍ Å¸ÀÔ)
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

				// ¼Ò¼öÁ¡ ¼ÂÂ° ÀÚ¸®¿¡¼­ ¹Ý¿Ã¸²
				dValue += 0.005;

				// ¼Ò¼ö ºÎºÐÀÇ ¼Ò¼öÁ¡ µÑÂ° ÀÚ¸®ºÎÅÍ Â÷·Ê´ë·Î ¹öÆÛ¿¡ ÀúÀå
				pcBuffer[iBufferIndex] = '0' + ((QWORD)(dValue * 100) % 10);
				pcBuffer[iBufferIndex + 1] = '0' + ((QWORD)(dValue * 10) % 10);
				pcBuffer[iBufferIndex + 2] = '.';

				for(k = 0; ; k++){
					// Á¤¼ö ºÎºÐÀÌ 0ÀÌ¸é, ·çÇÁ Á¾·á
					if(((QWORD)dValue == 0) && (k != 0)){
						break;
					}

					// Á¤¼ö ºÎºÐÀÇ ÀÏÀÇ ÀÚ¸®ºÎÅÍ Â÷·Ê´ë·Î ¹öÆÛ¿¡ ÀúÀå
					pcBuffer[iBufferIndex + 3 + k] = '0' + ((QWORD)dValue % 10);
					dValue = dValue / 10;
				}

				pcBuffer[iBufferIndex + 3 + k] = '\0';

				// ½Ç¼ö°¡ ÀúÀåµÈ ±æÀÌ ¸¸Å­ ¹öÆÛ¸¦ µÚÁý°í, ¹öÆÛ ÀÎµ¦½º¸¦ Áõ°¡½ÃÅ´
				kReverseString(pcBuffer + iBufferIndex);
				iBufferIndex += 3 + k;
				break;

			default:
				pcBuffer[iBufferIndex] = pcFormatString[i];
				iBufferIndex++;
				break;
			}

		// '%'°¡ ¾Æ´Ñ °æ¿ì (ÀÏ¹Ý ¹®ÀÚ¿­)
		}else{
			pcBuffer[iBufferIndex] = pcFormatString[i];
			iBufferIndex++;
		}
	}

	pcBuffer[iBufferIndex] = '\0';

	return iBufferIndex; // Ãâ·ÂÇÑ ¹®ÀÚ¿­ÀÇ ±æÀÌ¸¦ ¹ÝÈ¯
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

inline void kMemSetWord(void* pvDestination, WORD wData, int iWordSize){
	int i;
	QWORD qwData;
	int iRemainWordStartOffset;

	qwData = 0;
	// 8바이트에 데이터를 채움
	for(i=0; i<4; i++)
		qwData = (qwData << 16) | wData;

	// 8바이트 씩 채움
	for(i=0 ;i<(iWordSize/4); i++)
		((QWORD*)pvDestination)[i] = qwData;

	// 8 바이트씩 채우고 남은 부분을 마무리
	iRemainWordStartOffset = i*4;
	for(i=0; i<(iWordSize % 4); i++)
		((WORD*)pvDestination)[iRemainWordStartOffset++] = wData;
}
