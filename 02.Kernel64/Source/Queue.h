#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "Types.h"

#pragma pack(push, 1)

typedef struct kQueueManagerStruct{
	int iDataSize;
	int iMaxDataCount;
	
	void* pvQueueArray;
	int iPutIndex;
	int iGetIndex;

	BOOL bLastOperationPut;

}QUEUE;

#pragma pack(pop)


void kinitializeQueue(QUEUE*, void*, int, int);
BOOL kIsQueueFull(const QUEUE*);
BOOL kIsQueueEmpty(const QUEUE*);
BOOL kPutQueue(QUEUE*, const void*);
BOOL kGetQueue(QUEUE*, void*);

#endif