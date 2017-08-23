#ifndef __ASSEMBLYUTILITY_H__
#define __ASSEMBLYUTILITY_H__

#include "Types.h"
#include "Task.h"

// 함수 정의
BYTE kInPortByte(WORD wPort);
void kOutPortByte(WORD wPort, BYTE bData);
void kLoadGDTR(QWORD qwGDTRAddress);
void kLoadTR(WORD wTSSSegmentOffset);
void kLoadIDTR(QWORD qwIDTRAddress);
void kEnableInterrupt(void);
void kDisableInterrupt(void);
QWORD kReadRFLAGS(void);
QWORD kReadTSC(void);
void kSwitchContext(CONTEXT* , CONTEXT* );
void kHit(void);
BOOL kTestAndSet(volatile BYTE* pbDestination, BYTE Compare, BYTE source);
void kInitializeFPU(void);
void kSaveFPUContext(void* pvFPUContext);
void kLoadFPUContext(void* pvFPUContext);
void kSetTS(void);
void kClearTS(void);
WORD kInPortWord(WORD wPort);
WORD kOutPortWord(WORD wPort, WORD wData);

#endif // __ASSEMBLYUTILITY_H__
