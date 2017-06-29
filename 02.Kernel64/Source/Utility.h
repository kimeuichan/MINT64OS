#ifndef __UTILITY_H__
#define __UTILITY_H__

#include <stdarg.h>
#include "Types.h"

void kMemSet(void* pvDest, BYTE bData, int iSize);
int kMemCpy(void* pvDest, const void* pvSrc, int iSize);
int kMemCmp(const void* pvDest, const void* pvSrc, int iSize);
BOOL kSetInterruptFlag(BOOL bEnableInterrupt);
int kStrLen(const char* pcBuffer);
void kCheckTotalRAMSize(void);
QWORD kGetTotalRAMSize(void);
void kReverseString(char* pcBuffer);
long kAToI(const char* pcBuffer, int iRadix);
QWORD kHexStringToQword(const char* pcBuffer);
long kDecimalStringToLong(const char* pcBuffer);
int kIToA(long lValue, char* pcBuffer, int iRadix);
int kHexToString(QWORD pwValue, char* pcBuffer);
int kDecimalToString(long lValue, char* pcBuffer);
int kSPrintf(char* pcBuffer, const char* pcFormatString, ...);
int kVSPrintf(char* pcBuffer, const char* pcFormatString, va_list ap);
QWORD kGetTickCount(void);

extern volatile QWORD g_qwTickCount;
#endif // __UTILITY_H__
