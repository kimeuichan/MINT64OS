#ifndef __MULTIPROCESSOR_H__

#define __MULTIPROCESSOR_H__

#include "Types.h"

// 매크로
// MultiProcessor 관련
#define BOOTSTRAPPROCESSOR_FLAGADDRESS		0x7c09
#define MAXPROCESSORCOUNT					16

BOOL kStartUpApplicationProcessor(void);
static BOOL kWakeUpApplicationProcessor(void);
BYTE kGetAPICID(void);

#endif