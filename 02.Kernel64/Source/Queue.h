#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "Types.h"

// 구조체 정의
#pragma pack(push, 1)

typedef struct kQueueManagerStruct{
	int iDataSize;
	int iMaxDataCount;
	void* pvQueueArray; // 범용 큐를 위한 배열의 주소값 저장
	int iGetIndex;      // 배열의 머리(head)에서 삭제
	int iPutIndex;      // 배열의 꼬리(tail)에서 삽입
	BOOL bLastOperationPut;
} QUEUE;

#pragma pack(pop)

// 함수 정의
void kInitializeQueue(QUEUE* pstQueue, void* pvQueueBuffer, int iMaxDataCount, int iDataSize);
BOOL kIsQueueFull(const QUEUE* pstQueue);
BOOL kIsQueueEmpty(const QUEUE* pstQueue);
BOOL kPutQueue(QUEUE* pstQueue, const void* pvData);
BOOL kGetQueue(QUEUE* pstQueue, void* pvData);

#endif // __QUEUE_H__
