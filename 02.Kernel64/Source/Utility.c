#include "Utility.h"

void kPrintString(int iX, int iY, const char* pcString){
	CHARACTER* pstScreen = (CHARACTER*)0xB8000;
	int i;

	pstScreen += (iY*80) + iX;

	for(i = 0; pcString[i] != NULL; i++){
		pstScreen[i].bCharacter = pcString[i];
	}
}

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
