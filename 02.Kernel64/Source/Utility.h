#ifndef __UTILITY_H__
#define __UTILITY_H__

#include "Types.h"

void kPrintString(int iX, int iY, const char* pcString);
void kMemSet(void* pvDest, BYTE bData, int iSize);
int kMemCpy(void* pvDest, const void* pvSrc, int iSize);
int kMemCmp(const void* pvDest, const void* pvSrc, int iSize);
BOOL kSetInterruptFlag(BOOL);

#endif // __UTILITY_H__
